#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/pmap.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

static void cpu_thread_exception(struct thread *, void *);

void
cpu_thread_activate(struct thread *td)
{
	PCPU_SET(thread, td);

	if ((td->td_task->t_flags & TASK_KERNEL) == 0)
		pmap_activate(td->td_task->t_vm);
}

void
cpu_thread_free(struct thread *td)
{
	int error;

	error = vm_free(&kernel_vm, KSTACK_SIZE, td->td_kstack);
	if (error != 0)
		panic("%s: vm_free of kstack failed: %m", __func__, error);

	if ((td->td_task->t_flags & TASK_KERNEL) == 0) {
		error = vm_free(td->td_task->t_vm, MAILBOX_SIZE, td->td_cputhread.td_mbox);
		if (error != 0)
			panic("%s: vm_free of mboxfailed: %m", __func__, error);
	}
}

void
cpu_thread_set_upcall(struct thread *td, void (*function)(struct thread *, void *), void *arg)
{
	td->td_context.c_regs[CONTEXT_RA] = (uintptr_t)&cpu_thread_trampoline;
	td->td_context.c_regs[CONTEXT_S0] = (uintptr_t)td;
	td->td_context.c_regs[CONTEXT_S1] = (uintptr_t)function;
	td->td_context.c_regs[CONTEXT_S2] = (uintptr_t)arg;
	td->td_context.c_regs[CONTEXT_SP] = td->td_kstack + KSTACK_SIZE;
}

void
cpu_thread_set_upcall_user(struct thread *td, vaddr_t function, register_t arg)
{
	td->td_context.c_regs[CONTEXT_RA] = (uintptr_t)&cpu_thread_trampoline_user;
	td->td_context.c_regs[CONTEXT_S0] = (uintptr_t)td;
	td->td_context.c_regs[CONTEXT_S1] = (uintptr_t)function;
	td->td_context.c_regs[CONTEXT_S2] = (uintptr_t)arg;
	td->td_context.c_regs[CONTEXT_SP] = td->td_kstack + KSTACK_SIZE;
}

int
cpu_thread_setup(struct thread *td)
{
	vaddr_t kstack, mbox, ustack;
	unsigned off;
	int error;

	td->td_cputhread.td_frame = NULL;

	tlb_wired_init(&td->td_cputhread.td_tlbwired);
	cpu_thread_set_upcall(td, cpu_thread_exception, td);

	error = vm_alloc(&kernel_vm, KSTACK_SIZE, &kstack);
	if (error != 0)
		return (error);
	td->td_kstack = kstack;
	for (off = 0; off < KSTACK_SIZE; off += PAGE_SIZE)
		tlb_wired_wire(&td->td_cputhread.td_tlbwired, kernel_vm.vm_pmap,
			       kstack + off);
	memset(&td->td_context, 0, sizeof td->td_context);

	if ((td->td_task->t_flags & TASK_KERNEL) == 0) {
		error = vm_alloc(td->td_task->t_vm, MAILBOX_SIZE, &mbox);
		if (error != 0)
			return (error);
		td->td_cputhread.td_mbox = mbox;
		for (off = 0; off < MAILBOX_SIZE; off += MAILBOX_SIZE)
			tlb_wired_wire(&td->td_cputhread.td_tlbwired,
				       td->td_task->t_vm->vm_pmap, mbox + off);
	}

	if ((td->td_flags & THREAD_USTACK) != 0) {
		error = vm_alloc_address(td->td_task->t_vm, &ustack, USTACK_SIZE / PAGE_SIZE, true);
		if (error != 0)
			return (error);
		td->td_ustack_bottom = ustack;
		td->td_ustack_top = ustack + USTACK_SIZE;
	} else {
		td->td_ustack_top = 0;
		td->td_ustack_bottom = 0;
	}

	return (0);
}

static void
cpu_thread_exception(struct thread *td, void *arg)
{
	ASSERT(td == arg, ("thread and argument mismatch"));
	panic("%s: upcall not set on thread %p", __func__, (void *)td);
}

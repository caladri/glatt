#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_page.h>

#if 0
static void cpu_thread_exception(void *);
#endif

void
cpu_thread_set_upcall(struct thread *td, void (*function)(void *), void *arg)
{
	panic("%s: not yet implemented.", __func__);
}

void
cpu_thread_free(struct thread *td)
{
	panic("%s: not yet implemented.", __func__);
#if 0
	int error;

	error = vm_free(&kernel_vm, KSTACK_SIZE, td->td_kstack);
	if (error != 0)
		panic("%s: vm_free of kstack failed: %m", __func__, error);

	if ((td->td_task->t_flags & TASK_KERNEL) == 0) {
		error = vm_free(td->td_task->t_vm, MAILBOX_SIZE, td->td_cputhread.td_mbox);
		if (error != 0)
			panic("%s: vm_free of mboxfailed: %m", __func__, error);
	}
#endif
}

int
cpu_thread_setup(struct thread *td)
{
	panic("%s: not yet implemented.", __func__);
#if 0
	vaddr_t kstack, mbox;
	unsigned off;
	int error;

	tlb_wired_init(&td->td_cputhread.td_tlbwired);
	cpu_thread_set_upcall(td, cpu_thread_exception, td);

	error = vm_alloc(&kernel_vm, KSTACK_SIZE, &kstack);
	if (error != 0)
		return (error);
	td->td_kstack = (void *)kstack;
	for (off = 0; off < KSTACK_SIZE; off += PAGE_SIZE)
		tlb_wired_wire(&td->td_cputhread.td_tlbwired, kernel_vm.vm_pmap,
			       kstack + off);
	td->td_context.c_regs[CONTEXT_SP] = kstack + KSTACK_SIZE;

	if ((td->td_task->t_flags & TASK_KERNEL) == 0) {
		error = vm_alloc(td->td_task->t_vm, MAILBOX_SIZE, &mbox);
		if (error != 0)
			return (error);
		td->td_cputhread.td_mbox = mbox;
		for (off = 0; off < MAILBOX_SIZE; off += MAILBOX_SIZE)
			tlb_wired_wire(&td->td_cputhread.td_tlbwired,
				       td->td_task->t_vm->vm_pmap, mbox + off);
	}

	return (0);
#endif
}

void
cpu_thread_user_trampoline(void *entry)
{
	panic("%s: not yet implemented.", __func__);
}

#if 0
static void
cpu_thread_exception(void *arg)
{
	struct thread *td;

	td = arg;
	panic("%s: upcall not set on thread %p", __func__, (void *)arg);
}
#endif

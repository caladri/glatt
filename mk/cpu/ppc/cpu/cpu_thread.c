#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

#if 0
static void cpu_thread_exception(void *);
#endif

void
cpu_thread_set_upcall(struct thread *td, void (*function)(void *), void *arg)
{
	panic("%s: not yet implemented.", __func__);
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

	error = vm_alloc(td->td_task->t_vm, MAILBOX_SIZE, &mbox);
	if (error != 0)
		return (error);
	td->td_cputhread.td_mbox = mbox;
	for (off = 0; off < MAILBOX_SIZE; off += MAILBOX_SIZE)
		tlb_wired_wire(&td->td_cputhread.td_tlbwired,
			       td->td_task->t_vm->vm_pmap, mbox + off);

	return (0);
#endif
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

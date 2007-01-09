#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <core/thread.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

static void cpu_thread_exception(void *);

void
cpu_thread_set_upcall(struct thread *td, void (*function)(void *), void *arg)
{
	td->td_context.c_regs[CONTEXT_RA] = (uintptr_t)&thread_trampoline;
	td->td_context.c_regs[CONTEXT_S0] = (uintptr_t)td;
	td->td_context.c_regs[CONTEXT_S1] = (uintptr_t)function;
	td->td_context.c_regs[CONTEXT_S2] = (uintptr_t)arg;
}

int
cpu_thread_setup(struct thread *td)
{
	vaddr_t kstack;
	int error;

	error = vm_alloc(&kernel_vm, KSTACK_SIZE, &kstack);
	if (error != 0)
		return (error);
	td->td_kstack = (void *)kstack;
	/*
	 * Zero all of the kstack so that it's all already dirty, otherwise
	 * when we first try to write to it, we will end up triple faulting
	 * ad absurdum.  Trying to not use the kstack when servicing TLBMod
	 * for the kstack would be ideal.
	 *
	 * XXX What do we do if the kstack goes out of the TLB?  Seems like the
	 * access pattern should prevent that, but hey.
	 *
	 * XXX Just touch each page.
	 *
	 * XXX Maybe put kstack in wired entries.
	 */
	memset(td->td_kstack, 0x00, KSTACK_SIZE);
	td->td_context.c_regs[CONTEXT_SP] = kstack + KSTACK_SIZE;
	cpu_thread_set_upcall(td, cpu_thread_exception, td);
	return (0);
}

static void
cpu_thread_exception(void *arg)
{
	struct thread *td;

	td = arg;
	panic("%s: upcall not set on thread %p", __func__, (void *)arg);
}

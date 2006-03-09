#include <core/types.h>
#include <core/error.h>
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
	panic("%s: vm_alloc gave us %p\n", __func__, (void *)kstack);
	td->td_kstack = (void *)kstack;
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

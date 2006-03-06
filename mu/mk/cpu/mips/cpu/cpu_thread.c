#include <core/types.h>
#include <core/error.h>
#include <core/thread.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

int
cpu_thread_setup(struct thread *td)
{
	vaddr_t kstack;
	int error;

	error = vm_alloc(&kernel_vm, KSTACK_SIZE, &kstack);
	if (error != 0)
		return (error);
	td->td_kstack = (void *)kstack;
	/* XXX setup context.  */
	return (0);
}

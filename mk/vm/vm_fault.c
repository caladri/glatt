#include <core/types.h>
#include <core/error.h>
#include <core/task.h>
#include <core/thread.h>
#include <vm/vm.h>
#include <vm/vm_fault.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

int
vm_fault_stack(struct thread *td, vaddr_t vaddr)
{
	struct task *task;
	struct vm *vm;
	int error;

	task = td->td_task;
	vm = task->t_vm;

	vaddr = PAGE_FLOOR(vaddr);

	error = vm_alloc_range(vm, vaddr, vaddr + PAGE_SIZE);
	if (error != 0)
		return (error);

	error = page_alloc_map(vm, PAGE_FLAG_ZERO, vaddr);
	if (error != 0)
		return (error);

	return (0);
}

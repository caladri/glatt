#include <core/types.h>
#include <core/error.h>
#include <vm/vm.h>

struct vm kernel_vm;

int
vm_find_address(struct vm *vm, vaddr_t *vaddrp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_free_address(struct vm *vm, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

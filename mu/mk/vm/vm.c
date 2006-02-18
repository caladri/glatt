#include <core/types.h>
#include <core/error.h>
#include <vm/vm.h>

struct vm kernel_vm;

int
vm_alloc_address(struct vm *vm, vaddr_t *vaddrp, size_t pages)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_free_address(struct vm *vm, vaddr_t vaddr, size_t pages)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_insert_range(struct vm *vm, vaddr_t begin, vaddr_t end)
{
	return (ERROR_NOT_IMPLEMENTED);
}

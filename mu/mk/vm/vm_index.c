#include <core/types.h>
#include <core/error.h>
#include <vm/vm.h>

struct vm_index {
	vaddr_t vmi_base;
	size_t vmi_size;
	struct vm_index *vmi_left;
	struct vm_index *vmi_right;
};

struct vm kernel_vm;

int
vm_alloc_address(struct vm *vm, vaddr_t *vaddrp, size_t pages)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_free_address(struct vm *vm, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_insert_range(struct vm *vm, vaddr_t begin, vaddr_t end)
{
	return (ERROR_NOT_IMPLEMENTED);
}

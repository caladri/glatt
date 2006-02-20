#include <core/types.h>
#include <core/error.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

int
vm_alloc(struct vm *vm, size_t size, vaddr_t *vaddrp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_free(struct vm *vm, size_t size, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

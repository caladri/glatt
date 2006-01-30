#include <core/types.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

int
page_alloc(struct vm *vm, paddr_t *paddrp)
{
	return (-1);
}

int
page_alloc_virtual(struct vm *vm, vaddr_t *vaddrp)
{
	paddr_t paddr;
	vaddr_t vaddr;
	int error, error2;

	error = page_alloc(vm, &paddr);
	if (error != 0)
		return (error);

	error = vm_find_address(vm, &vaddr);
	if (error != 0) {
		error2 = page_release(vm, paddr);
		if (error2 != 0) {
			panic("%s: can't release paddr.", __func__);
		}
		return (error);
	}

	error = page_map(vm, paddr, vaddr);
	if (error != 0) {
		error2 = vm_free_address(vm, vaddr);
		if (error2 != 0) {
			panic("%s: can't release vaddr.", __func__);
		}
		return (error);
	}
	*vaddrp = vaddr;
	return (0);
}

int
page_insert(paddr_t paddr)
{
	return (-1);
}

int
page_map(struct vm *vm, vaddr_t vaddr, paddr_t paddr)
{
	return (-1);
}

int
page_release(struct vm *vm, paddr_t paddr)
{
	return (-1);
}

int
page_unmap(struct vm *vm, vaddr_t vaddr)
{
	return (-1);
}

#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

int
vm_alloc(struct vm *vm, size_t size, vaddr_t *vaddrp)
{
	size_t o, pages;
	paddr_t paddr;
	vaddr_t vaddr;
	int error, error2;

	if (size < pool_max_alloc)
		panic("%s: allocation too small, use pool instead.", __func__);
	pages = ADDR_TO_PAGE(size + (PAGE_SIZE - 1));
	error = vm_alloc_address(vm, &vaddr, pages);
	if (error != 0)
		return (error);
	for (o = 0; o < pages; o++) {
		error = page_alloc(vm, &paddr);
		if (error != 0) {
			while (o--) {
				error2 = page_extract(vm, vaddr + o * PAGE_SIZE,
						     &paddr);
				if (error2 != 0)
					panic("%s: failed to extract from mapping: %u",
					      __func__, error2);
			}
			error2 = vm_free_address(vm, vaddr);
			if (error2 != 0) {
				panic("%s: vm_free_address failed: %u",
				      __func__, error2);
			}
			return (error);
		}
		error = page_map(vm, vaddr + o * PAGE_SIZE, paddr);
		if (error != 0) {
			panic("%s: must free pages for failed mapping.", __func__);
		}
	}
	*vaddrp = vaddr;
	return (0);
}

int
vm_free(struct vm *vm, size_t size, vaddr_t vaddr)
{
	size_t o, pages;
	paddr_t paddr;
	int error;

	pages = size / PAGE_SIZE;
	if ((size % PAGE_SIZE) != 0)
		pages++;
	for (o = 0; o < pages; o++) {
		error = page_extract(vm, vaddr + o * PAGE_SIZE, &paddr);
		if (error != 0)
			panic("%s: failed to extract from mapping: %u",
			      __func__, error);
		error = page_release(vm, paddr);
		if (error != 0)
			panic("%s: failed to release page: %u", __func__,
			      error);
	}
	error = vm_free_address(vm, vaddr);
	if (error != 0)
		panic("%s: failed to free address: %u", __func__, error);
	return (0);
}

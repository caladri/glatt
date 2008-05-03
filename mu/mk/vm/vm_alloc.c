#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

int
vm_alloc(struct vm *vm, size_t size, vaddr_t *vaddrp)
{
	size_t o, pages;
	struct vm_page *page;
	vaddr_t vaddr;
	int error;

	if (size < pool_max_alloc)
		panic("%s: allocation too small, use pool instead.", __func__);
	pages = ADDR_TO_PAGE(size + (PAGE_SIZE - 1));
	error = vm_alloc_address(vm, &vaddr, pages);
	if (error != 0)
		return (error);
	for (o = 0; o < pages; o++) {
		error = page_alloc(vm, PAGE_FLAG_DEFAULT, &page);
		if (error != 0) {
			panic("%s: out of memory.", __func__);
		}
		error = page_map(vm, vaddr + o * PAGE_SIZE, page);
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
	struct vm_page *page;
	int error;

	pages = size / PAGE_SIZE;
	if ((size % PAGE_SIZE) != 0)
		pages++;
	for (o = 0; o < pages; o++) {
		error = vm_map_extract(vm, vaddr + o * PAGE_SIZE, &page);
		if (error != 0)
			panic("%s: failed to extract from mapping: %m",
			      __func__, error);
		error = page_unmap(vm, vaddr + o * PAGE_SIZE);
		if (error != 0)
			panic("%s: failed to release mapping: %m",
			      __func__, error);
		error = page_release(vm, page);
		if (error != 0)
			panic("%s: failed to release page: %m", __func__,
			      error);
	}
	error = vm_free_address(vm, vaddr);
	if (error != 0)
		panic("%s: failed to free address: %m", __func__, error);
	return (0);
}

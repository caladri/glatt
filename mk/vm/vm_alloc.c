#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <vm/alloc.h>
#include <vm/index.h>
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

#if !defined(VM_ALLOC_NO_DIRECT)
	if (vm == &kernel_vm && pages == 1) {
		return (page_alloc_direct(vm, PAGE_FLAG_DEFAULT, vaddrp));
	}
#endif

	error = vm_alloc_address(vm, &vaddr, pages);
	if (error != 0)
		return (error);
	for (o = 0; o < pages; o++) {
		error = page_alloc(PAGE_FLAG_DEFAULT, &page);
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
vm_alloc_page(struct vm *vm, vaddr_t *vaddrp)
{
	int error;

	error = vm_alloc(vm, PAGE_SIZE, vaddrp);
	if (error != 0)
		return (error);
	return (0);
}

int
vm_free(struct vm *vm, size_t size, vaddr_t vaddr)
{
	size_t o, pages;
	struct vm_page *page;
	int error;

	pages = ADDR_TO_PAGE(size + (PAGE_SIZE - 1));

#if !defined(VM_ALLOC_NO_DIRECT)
	if (vm == &kernel_vm && pages == 1) {
		return (page_free_direct(vm, vaddr));
	}
#endif

	for (o = 0; o < pages; o++) {
		error = page_extract(vm, vaddr + o * PAGE_SIZE, &page);
		if (error != 0)
			panic("%s: failed to extract from mapping: %m",
			      __func__, error);
		error = page_unmap(vm, vaddr + o * PAGE_SIZE, page);
		if (error != 0)
			panic("%s: failed to release mapping: %m",
			      __func__, error);
		error = page_release(page);
		if (error != 0)
			panic("%s: failed to release page: %m", __func__,
			      error);
	}
	error = vm_free_address(vm, vaddr);
	if (error != 0)
		panic("%s: failed to free address: %m", __func__, error);
	return (0);
}

int
vm_free_page(struct vm *vm, vaddr_t vaddr)
{
	int error;

	error = vm_free(vm, PAGE_SIZE, vaddr);
	if (error != 0)
		return (error);
	return (0);
}

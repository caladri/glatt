#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

int
vm_alloc(struct vm *vm, size_t size, vaddr_t *vaddrp)
{
	size_t o, pages;
	vaddr_t vaddr;
	int error;

	if (vm == &kernel_vm && size < pool_max_alloc)
		panic("%s: allocation too small, use pool instead.", __func__);

	pages = PAGE_COUNT(size);

#if !defined(VM_ALLOC_NO_DIRECT)
	if (vm == &kernel_vm && pages == 1)
		return (page_alloc_direct(vm, PAGE_FLAG_DEFAULT, vaddrp));
#endif

	error = vm_alloc_address(vm, &vaddr, pages, false);
	if (error != 0)
		return (error);
	for (o = 0; o < pages; o++) {
		error = page_alloc_map(vm, PAGE_FLAG_DEFAULT, vaddr + o * PAGE_SIZE);
		if (error != 0)
			panic("%s: page_alloc_map failed: %m", __func__, error);
	}
	*vaddrp = vaddr;
	return (0);
}

int
vm_alloc_page(struct vm *vm, vaddr_t *vaddrp)
{
	vaddr_t vaddr;
	int error;

#if !defined(VM_ALLOC_NO_DIRECT)
	if (vm == &kernel_vm)
		return (page_alloc_direct(vm, PAGE_FLAG_DEFAULT, vaddrp));
#endif

	error = vm_alloc_address(vm, &vaddr, 1, false);
	if (error != 0)
		return (error);

	error = page_alloc_map(vm, PAGE_FLAG_DEFAULT, vaddr);
	if (error != 0)
		panic("%s: page_alloc_map failed: %m", __func__, error);

	*vaddrp = vaddr;

	return (0);
}

int
vm_alloc_range_wire(struct vm *vm, vaddr_t begin, vaddr_t end, vaddr_t *kvaddrp, size_t *offp)
{
	int error;

	if (vm == &kernel_vm)
		panic("%s: can't wire from kernel to kernel.", __func__);

	error = vm_alloc_range(vm, begin, end);
	if (error != 0)
		return (error);

	error = vm_wire(vm, begin, end - begin, kvaddrp, offp, true);
	if (error != 0)
		panic("%s: vm_wire failed: %m", __func__, error);

	return (0);
}

int
vm_free(struct vm *vm, size_t size, vaddr_t vaddr)
{
	size_t o, pages;
	int error;

	pages = PAGE_COUNT(size);

#if !defined(VM_ALLOC_NO_DIRECT)
	if (vm == &kernel_vm && pages == 1)
		return (page_free_direct(vm, vaddr));
#endif

	for (o = 0; o < pages; o++) {
		error = page_free_map(vm, vaddr + o * PAGE_SIZE);
		if (error != 0)
			panic("%s: page_free_map failed: %m", __func__,
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

int
vm_wire(struct vm *vm, vaddr_t uvaddr, size_t len, vaddr_t *kvaddrp, size_t *offp, bool fault)
{
	struct vm_page *page;
	size_t o, pages;
	vaddr_t kvaddr;
	vaddr_t vaddr;
	int error;

	vaddr = PAGE_FLOOR(uvaddr);
	pages = PAGE_COUNT((uvaddr + len) - vaddr);

	if (vm == &kernel_vm)
		panic("%s: can't wire from kernel to kernel.", __func__);

#if !defined(VM_ALLOC_NO_DIRECT)
	if (pages == 1) {
		error = page_extract(vm, vaddr, &page);
		if (error != 0) {
			if (fault && error == ERROR_NOT_FOUND) {
				error = page_alloc(PAGE_FLAG_ZERO, &page);
				if (error != 0)
					panic("%s: page_alloc failed: %m", __func__, error);

				error = page_map(vm, vaddr, page);
				if (error != 0)
					panic("%s: page_map failed: %m", __func__, error);
			} else {
				panic("%s: page_extract failed: %m", __func__, error);
			}

		}

		error = page_map_direct(&kernel_vm, page, kvaddrp);
		if (error != 0)
			panic("%s: page_map_direct failed: %m", __func__, error);

		*offp = PAGE_OFFSET(uvaddr);
		return (0);
	}
#endif

	error = vm_alloc_address(&kernel_vm, &kvaddr, pages, false);
	if (error != 0)
		panic("%s: vm_alloc_address failed: %m", __func__, error);

	for (o = 0; o < pages; o++) {
		error = page_extract(vm, vaddr + o * PAGE_SIZE, &page);
		if (error != 0) {
			if (fault && error == ERROR_NOT_FOUND) {
				error = page_alloc(PAGE_FLAG_ZERO, &page);
				if (error != 0)
					panic("%s: page_alloc failed: %m", __func__, error);

				error = page_map(vm, vaddr + o * PAGE_SIZE, page);
				if (error != 0)
					panic("%s: page_map failed: %m", __func__, error);
			} else {
				panic("%s: page_extract failed: %m", __func__, error);
			}

		}

		error = page_map(&kernel_vm, kvaddr + o * PAGE_SIZE, page);
		if (error != 0)
			panic("%s: page_map (kernel_vm) failed: %m", __func__, error);
	}

	*kvaddrp = kvaddr;
	*offp = PAGE_OFFSET(uvaddr);

	return (0);
}

int
vm_unwire(struct vm *vm, vaddr_t uvaddr, size_t len, vaddr_t kvaddr)
{
	struct vm_page *page;
	size_t o, pages;
	vaddr_t vaddr;
	int error;

	vaddr = PAGE_FLOOR(uvaddr);
	pages = PAGE_COUNT((uvaddr + len) - vaddr);

	if (vm == &kernel_vm)
		panic("%s: can't unwire from kernel to kernel.", __func__);

#if !defined(VM_ALLOC_NO_DIRECT)
	if (pages == 1) {
		error = page_extract(vm, vaddr, &page);
		if (error != 0)
			panic("%s: page_extract failed: %m", __func__, error);

		error = page_unmap_direct(&kernel_vm, page, kvaddr);
		if (error != 0)
			panic("%s: page_map_direct failed: %m", __func__, error);

		return (0);
	}
#endif

	for (o = 0; o < pages; o++) {
		error = page_extract(&kernel_vm, kvaddr + o * PAGE_SIZE, &page);
		if (error != 0)
			panic("%s: page_extract failed: %m", __func__, error);

		error = page_unmap(&kernel_vm, kvaddr + o * PAGE_SIZE, page);
		if (error != 0)
			panic("%s: page_unmap failed: %m", __func__, error);
	}

	error = vm_free_address(&kernel_vm, kvaddr);
	if (error != 0)
		panic("%s: failed to free address: %m", __func__, error);

	return (0);
}

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

	error = vm_alloc_address(vm, &vaddr, pages);
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

	error = vm_alloc_address(vm, &vaddr, 1);
	if (error != 0)
		return (error);

	error = page_alloc_map(vm, PAGE_FLAG_DEFAULT, vaddr);
	if (error != 0)
		panic("%s: page_alloc_map failed: %m", __func__, error);

	*vaddrp = vaddr;

	return (0);
}

int
vm_alloc_range_wire(struct vm *vm, vaddr_t begin, vaddr_t end, vaddr_t *kvaddrp)
{
	struct vm_page *page;
	size_t o, pages;
	vaddr_t kvaddr;
	vaddr_t vaddr;
	int error;

	vaddr = PAGE_FLOOR(begin);
	pages = PAGE_COUNT(end - vaddr);

	if (vm == &kernel_vm)
		panic("%s: can't wire from kernel to kernel.", __func__);

	error = vm_alloc_range(vm, begin, end);
	if (error != 0)
		return (error);

	error = vm_alloc_address(&kernel_vm, &kvaddr, pages);
	if (error != 0)
		panic("%s: vm_alloc_address failed: %m", __func__, error);

	for (o = 0; o < pages; o++) {
		error = page_alloc(PAGE_FLAG_DEFAULT, &page);
		if (error != 0)
			panic("%s: out of memory.", __func__);

		error = page_map(vm, vaddr + o * PAGE_SIZE, page);
		if (error != 0)
			panic("%s: page_map failed: %m", __func__, error);

		error = page_map(&kernel_vm, kvaddr + o * PAGE_SIZE, page);
		if (error != 0)
			panic("%s: page_map (kernel_vm) failed: %m", __func__, error);
	}

	*kvaddrp = kvaddr;

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
vm_free_range_wire(struct vm *vm, vaddr_t begin, vaddr_t end, vaddr_t kvaddr)
{
	struct vm_page *page;
	size_t o, pages;
	vaddr_t vaddr;
	int error;

	vaddr = PAGE_FLOOR(begin);
	pages = PAGE_COUNT(end - vaddr);

	if (vm == &kernel_vm)
		panic("%s: can't unwire from kernel to kernel.", __func__);

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

#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/string.h>
#include <vm/alloc.h>
#include <vm/copy.h>
#include <vm/index.h>
#include <vm/page.h>
#include <vm/vm.h>

static int vm_copyto(struct vm *, vaddr_t, size_t, struct vm *, vaddr_t);

int
vm_copy(struct vm *vms, vaddr_t saddr, size_t size,
	struct vm *vmd, vaddr_t *daddrp)
{
	int error, error2;
	vaddr_t daddr;
	size_t pages;

	pages = ADDR_TO_PAGE(size + (PAGE_SIZE - 1));

	error = vm_alloc_address(vmd, &daddr, pages);
	if (error != 0)
		return (error);

	error = vm_copyto(vms, saddr, size, vmd, daddr);
	if (error != 0) {
		error2 = vm_free_address(vmd, daddr);
		if (error2 != 0)
			panic("%s: vm_free_address failed: %m", __func__,
			      error2);
		return (error);
	}

	*daddrp = daddr;

	return (0);
}

static int
vm_copyto(struct vm *vms, vaddr_t saddr, size_t size,
	  struct vm *vmd, vaddr_t daddr)
{
	vaddr_t kaddr, taddr, dst, src;
	struct vm_page *spage, *dpage;
	size_t pages, amt;
	unsigned off, pg;
	const void *srcp;
	void *dstp;
	int error;

	/*
	 * XXX
	 * Really this isn't true but it's a reasonable restriction.
	 * If this becomes a pain (e.g. for copyout) then I can very
	 * easily modify this to not be necessary.
	 */
	ASSERT(PAGE_ALIGNED(daddr), "Destination must be page-aligned.");

	/* Allocate enough address space to copy the whole thing.  */
	pages = ADDR_TO_PAGE(size + (PAGE_SIZE - 1));
	off = PAGE_OFFSET(saddr);

	/*
	 * XXX
	 * We need not go through the kernel like this if the source
	 * is page-aligned, since we can pretty efficiently use the
	 * direct map, but that's hardly the best way, so for now go
	 * the easy way and:
	 *
	 * 1) Allocate address space at the destination.
	 * 2) Allocate address space for temporary use in the kernel.
	 * 3) Allocate pages and map them in the kernel.
	 * ...for each page...
	 * 5) Direct-map the source page.
	 * 6) Copy from the direct-mapped source page to KVA (skipping
	 *    any leading offset and any trailing data.)
	 * 7) Unmap source page.
	 * ...end for...
	 * 8) Move mappings from KVA to destination.
	 * 9) Free address space for temporary use in the kernel.
	 *
	 * XXX 2
	 * All failure handling could be better, but that's true for
	 * all of the microkernel.
	 *
	 * XXX 3
	 * No need for the kaddr allocation if vmd is kernel_vm.  Likewise
	 * no need for direct map if vms is kernel_vm.
	 */

	error = vm_alloc_address(&kernel_vm, &kaddr, pages);
	if (error != 0)
		return (error);

	for (pg = 0; pg < pages; pg++) {
		error = page_alloc(PAGE_FLAG_DEFAULT, &dpage);
		if (error != 0)
			panic("%s: page_alloc failed: %m", __func__,
			      error);

		error = page_map(&kernel_vm, kaddr + pg * PAGE_SIZE, dpage);
		if (error != 0)
			panic("%s: page_map failed: %m", __func__,
			      error);
	}

	dst = kaddr;
	for (src = PAGE_FLOOR(saddr); src < saddr + size; src += PAGE_SIZE) {
		error = page_extract(vms, src, &spage);
		if (error != 0)
			panic("%s: page_extract failed: %m", __func__, error);

		error = page_map_direct(&kernel_vm, spage, &taddr);
		if (error != 0)
			panic("%s: page_map_direct failed: %m", __func__,
			      error);

		srcp = (const void *)(taddr+ off);
		dstp = (void *)dst;

		if (off == 0) {
			memcpy(dstp, srcp, PAGE_SIZE);
			dst += PAGE_SIZE;
		} else {
			amt = size > (PAGE_SIZE - off) ? PAGE_SIZE - off : size;
			memcpy(dstp, srcp, amt);
			dst += off;
			off = 0;
		}

		error = page_unmap_direct(&kernel_vm, spage, taddr);
		if (error != 0)
			panic("%s: page_unmap_direct failed: %m", __func__,
			      error);
	}

	for (pg = 0; pg < pages; pg++) {
		error = page_extract(&kernel_vm, kaddr + pg * PAGE_SIZE,
				     &dpage);
		if (error != 0)
			panic("%s: page_extract failed: %m", __func__, error);

		error = page_unmap(&kernel_vm, kaddr + pg * PAGE_SIZE,
				   dpage);
		if (error != 0)
			panic("%s: page_unmap failed: %m", __func__, error);

		error = page_map(vmd, kaddr + pg * PAGE_SIZE, dpage);
		if (error != 0)
			panic("%s: page_map failed: %m", __func__, error);
	}

	error = vm_free_address(&kernel_vm, kaddr);
	if (error != 0)
		panic("%s: vm_free_address failed: %m", __func__, error);

	return (0);
}

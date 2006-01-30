#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

int
pmap_extract(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
	if (vaddr >= XKSEG_BASE && vaddr <= XKSEG_END) {
		if (vm != &kernel_vm)
			return (ERROR_NOT_PERMITTED);
		return (ERROR_NOT_IMPLEMENTED);
	}
	if (vaddr >= XUSEG_BASE && vaddr <= XUSEG_END) {
		return (ERROR_NOT_IMPLEMENTED);
	}
	/*
	 * XXX Check that it's actually an XKPHYS address, or at least be
	 * super-sure that it has to be.
	 */
	*paddrp = XKPHYS_EXTRACT(vaddr);
	return (0);
}

int
pmap_map(struct vm *vm, vaddr_t vaddr, paddr_t paddr)
{
	if (vm != &kernel_vm)
		panic("%s: not implemented.", __func__);
	return (ERROR_NOT_IMPLEMENTED);
}

int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	if (vm != &kernel_vm)
		panic("%s: not implemented.", __func__);
	*vaddrp = (vaddr_t)XKPHYS_MAP(XKPHYS_UC, paddr);
	return (0);
}

int
pmap_unmap(struct vm *vm, vaddr_t vaddr)
{
	if (vm != &kernel_vm)
		panic("%s: not implemented.", __func__);
	return (ERROR_NOT_IMPLEMENTED);
}

int
pmap_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
	if (vm != &kernel_vm)
		panic("%s: not implemented.", __func__);
	/* Don't have to do anything to get rid of direct-mapped memory.  */
	return (0);
}

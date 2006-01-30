#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

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
pmap_unmap_direct(struct vm *, vaddr_t vaddr)
{
	/* Don't have to do anything to get rid of direct-mapped memory.  */
	return (0);
}

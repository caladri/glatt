#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

int
pmap_map(struct vm *vm, vaddr_t vaddr, paddr_t paddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	*vaddrp = (vaddr_t)XKPHYS_MAP(XKPHYS_UC, paddr);
	return (0);
}

int
pmap_unmap(struct vm *vm, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

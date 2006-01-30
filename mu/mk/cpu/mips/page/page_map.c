#include <core/types.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	if (vm != &kernel_vm)
		panic("%s: can't direct-map for user address space.", __func__);
	*vaddrp = (vaddr_t)XKPHYS_MAP(XKPHYS_UC, paddr);
	return (0);
}

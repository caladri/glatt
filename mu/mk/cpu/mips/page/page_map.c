#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <cpu/tlb.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <page/page_map.h>
#include <vm/page.h>
#include <vm/vm.h>

static int pmap_alloc_pte(struct pmap *, vaddr_t, pt_entry_t **);
static void pmap_collect(struct pmap *);
static struct pmap_lev0 *pmap_find0(struct pmap *, vaddr_t);
static struct pmap_lev1 *pmap_find1(struct pmap_lev0 *, vaddr_t);
static struct pmap_lev2 *pmap_find2(struct pmap_lev1 *, vaddr_t);
static pt_entry_t *pmap_find_pte(struct pmap_lev2 *, vaddr_t);
static bool pmap_is_direct(vaddr_t);
static void pmap_pinit(struct pmap *, vaddr_t, vaddr_t);
static void pmap_update(struct pmap *, vaddr_t, paddr_t, pt_entry_t);

static struct pool pmap_pool;

unsigned
pmap_asid(struct pmap *pm)
{
	if (pm == NULL)
		return (0); /* XXX invalid ASID? */
	ASSERT(pm == kernel_vm.vm_pmap, "only support kernel address space");
	return (0);
}

void
pmap_bootstrap(void)
{
	int error;

	kcprintf("PMAP: %u level 0 pointers in each pmap.\n", NL0PMAP);
	kcprintf("PMAP: %lu level 1 pointers in each level 0 page.\n", NL1PL0);
	kcprintf("PMAP: %lu level 2 pointers in each level 1 page.\n", NL2PL1);
	kcprintf("PMAP: %lu PTEs in each level 2 page.\n", NPTEL2);
	kcprintf("PMAP: Level 2 maps %lu pages of virtual address space.\n",
		 NPTEL2);
	kcprintf("PMAP: Level 1 maps %lu pages of virtual address space.\n",
		 NPTEL2 * NL2PL1);
	kcprintf("PMAP: Level 0 maps %lu pages of virtual address space.\n",
		 NPTEL2 * NL2PL1 * NL1PL0);
	kcprintf("PMAP: Each pmap maps %lu pages of virtual address space.\n",
		 NPTEL2 * NL2PL1 * NL1PL0 * NL0PMAP);
	kcprintf("PMAP: Each pmap maps %luM (%luG) of virtual address space.\n",
		 ((NPTEL2 * NL2PL1 * NL1PL0 * NL0PMAP * PAGE_SIZE) /
		  (1024 * 1024)),
		 ((NPTEL2 * NL2PL1 * NL1PL0 * NL0PMAP * PAGE_SIZE) /
		  (1024 * 1024 * 1024)));
	error = pool_create(&pmap_pool, "PMAP", sizeof (struct pmap),
			    POOL_DEFAULT);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	error = pmap_init(&kernel_vm, KERNEL_BASE, KERNEL_END);
	if (error != 0)
		panic("%s: pmap_init failed: %m", __func__, error);
}

int
pmap_extract(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
	struct pmap *pm;
	pt_entry_t *pte;

	pm = vm->vm_pmap;

	vaddr &= ~PAGE_MASK;

	if (pmap_is_direct(vaddr)) {
		if (vm != &kernel_vm)
			return (ERROR_NOT_PERMITTED);
		*paddrp = XKPHYS_EXTRACT(vaddr);
		return (0);
	}

	if (vaddr >= pm->pm_end || vaddr < pm->pm_base)
		return (ERROR_NOT_PERMITTED);
	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		return (ERROR_NOT_FOUND);
	*paddrp = TLBLO_PTE_TO_PA(*pte);
	return (0);
}

pt_entry_t *
pmap_find(struct pmap *pm, vaddr_t vaddr)
{
	struct pmap_lev0 *pml0;
	struct pmap_lev1 *pml1;
	struct pmap_lev2 *pml2;

	if (vaddr < pm->pm_base || vaddr >= pm->pm_end)
		return (NULL);

	pml0 = pmap_find0(pm, vaddr);
	if (pml0 == NULL)
		return (NULL);
	pml1 = pmap_find1(pml0, vaddr);
	if (pml1 == NULL)
		return (NULL);
	pml2 = pmap_find2(pml1, vaddr);
	if (pml2 == NULL)
		return (NULL);
	return (pmap_find_pte(pml2, vaddr));
}

int
pmap_init(struct vm *vm, vaddr_t base, vaddr_t end)
{
	struct pmap *pm;
	int error;

	pm = pool_allocate(&pmap_pool);
	if (pm == NULL)
		return (ERROR_EXHAUSTED);
	vm->vm_pmap = pm;
	pmap_pinit(vm->vm_pmap, base, end);

	error = vm_insert_range(vm, base, end);
	if (error != 0) {
		pool_free(&pmap_pool, pm);
		return (error);
	}
	return (0);
}

int
pmap_map(struct vm *vm, vaddr_t vaddr, paddr_t paddr)
{
	struct pmap *pm;
	pt_entry_t *pte, flags;
	int error;

	pm = vm->vm_pmap;

	if (vaddr >= pm->pm_end || vaddr < pm->pm_base)
		return (ERROR_NOT_PERMITTED);

	error = pmap_alloc_pte(pm, vaddr, &pte);
	if (error != 0) {
		/* XXX mark that the pmap needs pmap_collect run.  */
		return (error);
	}
	flags = PG_V;
	if (vm == &kernel_vm)
		flags |= PG_G;
	/* Cache? */
	flags |= PG_C_CCEOW;
	pmap_update(pm, vaddr, paddr, flags);
	return (0);
}

int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	if (vm != &kernel_vm)
		return (ERROR_NOT_IMPLEMENTED);
	*vaddrp = (vaddr_t)XKPHYS_MAP(XKPHYS_CCEW, paddr);
	return (0);
}

int
pmap_unmap(struct vm *vm, vaddr_t vaddr)
{
	struct pmap *pm;
	pt_entry_t *pte;

	pm = vm->vm_pmap;

	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		return (ERROR_NOT_FOUND);
	/* Invalidate by updating to not have PG_V set.  */
	pmap_update(pm, vaddr, 0, 0);
	return (0);
}

int
pmap_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
	if (vm != &kernel_vm)
		return (ERROR_NOT_IMPLEMENTED);
	/* Don't have to do anything to get rid of direct-mapped memory.  */
	return (0);
}

static int
pmap_alloc_pte(struct pmap *pm, vaddr_t vaddr, pt_entry_t **ptep)
{
	struct pmap_lev0 *pml0;
	struct pmap_lev1 *pml1;
	struct pmap_lev2 *pml2;
	uint32_t pml0i, pml1i, pml2i;
	vaddr_t tmpaddr;
	int error;

	/* We have to have a pmap before getting here.  */
	if (pm == NULL)
		panic("%s: allocating into NULL pmap.", __func__);

	pml0i = pmap_index0(vaddr);
	if (pm->pm_level0[pml0i] == NULL) {
		error = page_alloc_direct(&kernel_vm, &tmpaddr);
		if (error != 0)
			return (error);
		pm->pm_level0[pml0i] = (struct pmap_lev0 *)tmpaddr;
	}
	pml0 = pmap_find0(pm, vaddr);
	pml1i = pmap_index1(vaddr);
	if (pml0->pml0_level1[pml1i] == NULL) {
		error = page_alloc_direct(&kernel_vm, &tmpaddr);
		if (error != 0) {
			/* XXX deallocate.  */
			return (error);
		}
		pml0->pml0_level1[pml1i] = (struct pmap_lev1 *)tmpaddr;
	}
	pml1 = pmap_find1(pml0, vaddr);
	pml2i = pmap_index2(vaddr);
	if (pml1->pml1_level2[pml2i] == NULL) {
		error = page_alloc_direct(&kernel_vm, &tmpaddr);
		if (error != 0) {
			/* XXX deallocate.  */
			return (error);
		}
		pml1->pml1_level2[pml2i] = (struct pmap_lev2 *)tmpaddr;
	}
	pml2 = pmap_find2(pml1, vaddr);
	if (ptep != NULL)
		*ptep = pmap_find_pte(pml2, vaddr);
	return (0);
}

static void
pmap_collect(struct pmap *pm)
{
	panic("%s: can't garbage collect yet.", __func__);
}

static struct pmap_lev0 *
pmap_find0(struct pmap *pm, vaddr_t vaddr)
{
	if (pm == NULL)
		return (NULL);
	return (pm->pm_level0[pmap_index0(vaddr)]);
}

static struct pmap_lev1 *
pmap_find1(struct pmap_lev0 *pml0, vaddr_t vaddr)
{
	if (pml0 == NULL)
		return (NULL);
	return (pml0->pml0_level1[pmap_index1(vaddr)]);
}

static struct pmap_lev2 *
pmap_find2(struct pmap_lev1 *pml1, vaddr_t vaddr)
{
	if (pml1 == NULL)
		return (NULL);
	return (pml1->pml1_level2[pmap_index2(vaddr)]);
}


static pt_entry_t *
pmap_find_pte(struct pmap_lev2 *pml2, vaddr_t vaddr)
{
	if (pml2 == NULL)
		return (NULL);
	return (&pml2->pml2_entries[pmap_index_pte(vaddr)]);
}

static bool
pmap_is_direct(vaddr_t vaddr)
{
	if (vaddr >= XKPHYS_BASE && vaddr <= XKPHYS_END)
		return (true);
	return (false);
}

static void
pmap_pinit(struct pmap *pm, vaddr_t base, vaddr_t end)
{
	unsigned l0;

	pm->pm_base = base;
	pm->pm_end = end;
	for (l0 = 0; l0 < NL0PMAP; l0++) {
		pm->pm_level0[l0] = NULL;
	}
}

static void
pmap_update(struct pmap *pm, vaddr_t vaddr, paddr_t paddr, pt_entry_t flags)
{
	pt_entry_t *pte;
	paddr_t opaddr;

	paddr &= ~PAGE_MASK;
	vaddr &= ~PAGE_MASK;

	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		panic("%s: update of PTE that isn't there.", __func__);

	opaddr = TLBLO_PTE_TO_PA(*pte);
	if (pte_test(pte, PG_V)) {
		if (opaddr == paddr) {
			/* Mapping stayed the same, just check flags.  */
			panic("%s: mapping stayed the same.", __func__);
			return;
		}
		/* XXX flush TLB, clear cache.  */
		tlb_invalidate(pm, vaddr);
	}
	*pte = TLBLO_PA_TO_PFN(paddr) | flags;
}

#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <page/page_table.h>
#include <vm/page.h>
#include <vm/vm.h>

/*
 * MIPS implements a simple 3-level page table.  The first level is just a
 * page-sized array of pointers to the second level, the second level is more
 * of the same, the third level is PTEs.  No reference counting or any such
 * silliness for now, since we don't particularly care if deallocating address
 * space is slow, we just want searching and implementation to be fast and
 * painless.  We could be more memory-efficient by only storing the meaningful
 * bits of pointers, and by using page numbers instead of real addresses, but
 * who wants to bother?
 *
 * Eventually one would like the first level to be at a fixed virtual address.
 *
 * Note that for MIPS3+, we get a two-page TLBLO, but it's better for us to use,
 * for example, 8K virtual pages and 4K physical pages and just do a little math
 * to get the second page, than to have the virtual page size and the physical
 * page size match and have to use a smaller (or too large) page size.
 *
 * XXX this whole thing needs to take into account userland allocating KVA.
 * maybe one sets of page tables for each address space?  yuck, though!
 * that's probably, best, though, then the bottom half of syscalls, etc.,
 * can used fixed-addresses without lots of indirect pointers.
 */

	/* Number of PTEs in Level 2.  */
#define	NPTEL2		(PAGE_SIZE / sizeof (pt_entry_t))

	/* Number of Level 2 pointers in Level 1.  */
#define	NL2PL1		(PAGE_SIZE / sizeof (struct pmap_lev2 *))

	/* Number of Level 1 pointers in Level 0.  */
#define	NL1PL0		(PAGE_SIZE / sizeof (struct pmap_lev1 *))

	/* Number of Level 0 pages required to map an entire address space.  */
	/*
	 * Each Level 2 page maps NPTEL2 virtual pages.  Each Level 1 page maps
	 * NPTEL2 * NL2PL1 pages.  Each Level 0 page maps NPTEL2 * NL2PL1 *
	 * NL1PL0 pages.  Using back-of-the-napkin math, one Level 0 page is
	 * enough to map 16G of address space, which should be enough for
	 * anyone.  
	 */
#define	NL0PMAP		(1) 

struct pmap_lev2 {
	pt_entry_t pml2_entries[NPTEL2];
};

struct pmap_lev1 {
	struct pmap_lev2 *pml1_level2[NL2PL1];
};

struct pmap_lev0 {
	struct pmap_lev1 *pml0_level1[NL1PL0];
};

struct pmap {
	struct pmap_lev0 *pm_level0[NL0PMAP];
	vaddr_t pm_base;
	vaddr_t pm_end;
};

static __inline vaddr_t
pmap_index_pte(vaddr_t vaddr)
{
	return ((vaddr / PAGE_SIZE) % NPTEL2);
}

static __inline vaddr_t
pmap_index2(vaddr_t vaddr)
{
	return ((vaddr / (PAGE_SIZE * NPTEL2)) % NL2PL1);
}

static __inline vaddr_t
pmap_index1(vaddr_t vaddr)
{
	return ((vaddr / (PAGE_SIZE * NPTEL2 * NL2PL1)) % NL1PL0);
}

static __inline vaddr_t
pmap_index0(vaddr_t vaddr)
{
	return ((vaddr / (PAGE_SIZE * NPTEL2 * NL2PL1 * NL1PL0)) % NL0PMAP);
}

static int pmap_alloc_pte(struct pmap *, vaddr_t, pt_entry_t **);
static void pmap_collect(struct pmap *);
static pt_entry_t *pmap_find(struct pmap *, vaddr_t);
static struct pmap_lev0 *pmap_find0(struct pmap *, vaddr_t);
static struct pmap_lev1 *pmap_find1(struct pmap_lev0 *, vaddr_t);
static struct pmap_lev2 *pmap_find2(struct pmap_lev1 *, vaddr_t);
static pt_entry_t *pmap_find_pte(struct pmap_lev2 *, vaddr_t);
static bool pmap_is_direct(vaddr_t);
static void pmap_pinit(struct pmap *, vaddr_t, vaddr_t);
static void pmap_update(pt_entry_t *, paddr_t); /* XXX protection.  */

void
pmap_bootstrap(void)
{
	vaddr_t vaddr;
	int error;
	
	error = page_alloc_direct(&kernel_vm, &vaddr);
	if (error != 0)
		panic("%s: page_alloc_direct failed: %u", __func__, error);
	/* XXX map this at a fixed virtual address.  */
	kernel_vm.vm_pmap = (struct pmap *)vaddr;
	pmap_pinit(kernel_vm.vm_pmap, XKSEG_BASE, XKSEG_END);
}

int
pmap_extract(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
	struct pmap *pm;
	pt_entry_t *pte;

	pm = vm->vm_pmap;

	vaddr &= PAGE_MASK;

	if (pmap_is_direct(vaddr)) {
		if (vm != &kernel_vm)
			return (ERROR_NOT_PERMITTED);
		*paddrp = XKPHYS_EXTRACT(vaddr);
		return (0);
	}

	if (vaddr >= pm->pm_end || vaddr <= pm->pm_base)
		return (ERROR_NOT_PERMITTED);
	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		return (ERROR_NOT_FOUND);
	*paddrp = TLBLO_PTE_TO_PA(*pte);
	return (0);
}

int
pmap_map(struct vm *vm, vaddr_t vaddr, paddr_t paddr)
{
	struct pmap *pm;
	pt_entry_t *pte;
	int error;

	pm = vm->vm_pmap;

	error = pmap_alloc_pte(pm, vaddr, &pte);
	if (error != 0) {
		/* XXX mark that the pmap needs pmap_collect run.  */
		return (error);
	}
	pmap_update(pte, paddr);
	return (0);
}

int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	if (vm != &kernel_vm)
		panic("%s: not implemented for userland.", __func__);
	*vaddrp = (vaddr_t)XKPHYS_MAP(XKPHYS_UC, paddr);
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
	*pte = 0;
	/* tlb invalidate, cache flush.  */
	return (0);
}

int
pmap_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
	if (vm != &kernel_vm)
		panic("%s: not implemented for userland.", __func__);
	/* Don't have to do anything to get rid of direct-mapped memory.  */
	return (0);
}

static int
pmap_alloc_pte(struct pmap *pm, vaddr_t vaddr, pt_entry_t **ptep)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static void
pmap_collect(struct pmap *pm)
{
	panic("%s: can't garbage collect yet.");
}

static pt_entry_t *
pmap_find(struct pmap *pm, vaddr_t vaddr)
{
	struct pmap_lev0 *pml0;
	struct pmap_lev1 *pml1;
	struct pmap_lev2 *pml2;

	vaddr -= pm->pm_base;

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
pmap_update(pt_entry_t *pte, paddr_t paddr)
{
	/*
	 * XXX invalidate old entry, flush TLB, let TLB layer destroy cache.
	 */
	panic("%s: not implemented.");
}

#ifndef	_PAGE_PAGE_MAP_H_
#define	_PAGE_PAGE_MAP_H_

#include <page/page_table.h>
#include <vm/page.h>

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
 * Even better if we weren't using the direct-map, and the pages self-mapped.
 * That said, it makes sense (perhaps) to keep using the direct map, and to
 * keep a shadow data structure which says which page table entries are paged
 * out, and then we can page them back in and fixup the page table as needed
 * (and just keep the entries NULL for anything not in main memory).  This
 * adds some complexity and overhead, but maybe it's on par with the overhead
 * and complexity of dealing with the page table levels themselves being in
 * kernel virtual address space.
 *
 * Note that for MIPS3+, we get a two-page TLBLO, but it's better for us to use,
 * for example, 8K virtual pages and 4K physical pages and just do a little math
 * to get the second page, than to have the virtual page size and the physical
 * page size match and have to use a smaller (or too large) page size.
 */

	/* Number of PTEs in Level 2.  */
#define	NPTEL2		(PAGE_SIZE / sizeof (pt_entry_t))
#define	PTEL2DIV	(PAGE_SIZE)

	/* Number of Level 2 pointers in Level 1.  */
#define	NL2PL1		(PAGE_SIZE / sizeof (struct pmap_lev2 *))
#define	L2L1DIV		(PTEL2DIV * NPTEL2)

	/* Number of Level 1 pointers in Level 0.  */
#define	NL1PL0		(PAGE_SIZE / sizeof (struct pmap_lev1 *))
#define	L1L0DIV		(L2L1DIV * NL2PL1)

	/* Number of Level 0 pages required to map an entire address space.  */
#define	NL0PMAP		(1) 
#define	PMAPL0DIV	(L1L0DIV * NL1PL0)

struct pmap_lev2 {
	pt_entry_t pml2_entries[NPTEL2];
};

COMPILE_TIME_ASSERT(sizeof (struct pmap_lev2) == PAGE_SIZE);

struct pmap_lev1 {
	struct pmap_lev2 *pml1_level2[NL2PL1];
};

COMPILE_TIME_ASSERT(sizeof (struct pmap_lev1) == PAGE_SIZE);

struct pmap_lev0 {
	struct pmap_lev1 *pml0_level1[NL1PL0];
};

COMPILE_TIME_ASSERT(sizeof (struct pmap_lev0) == PAGE_SIZE);

struct pmap {
	struct pmap_lev0 *pm_level0[NL0PMAP];
	vaddr_t pm_base;
	vaddr_t pm_end;
};

/*
 * If it goes over PAGE_SIZE, we would need a way to allocate a contiguous
 * direct-mapped region, or we would have to have the page tables self-map the
 * pmap entry.
 */
COMPILE_TIME_ASSERT(sizeof (struct pmap) <= PAGE_SIZE);

static __inline vaddr_t
pmap_index_pte(vaddr_t vaddr)
{
	return ((vaddr / PTEL2DIV) % NPTEL2);
}

static __inline vaddr_t
pmap_index2(vaddr_t vaddr)
{
	return ((vaddr / L2L1DIV) % NL2PL1);
}

static __inline vaddr_t
pmap_index1(vaddr_t vaddr)
{
	return ((vaddr / L1L0DIV) % NL1PL0);
}

static __inline vaddr_t
pmap_index0(vaddr_t vaddr)
{
	return ((vaddr / PMAPL0DIV) % NL0PMAP);
}

	/* Specifics of the MIPS PMAP.  */

unsigned pmap_asid(struct pmap *);

#endif /* !_PAGE_PAGE_MAP_H_ */

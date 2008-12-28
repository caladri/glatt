#ifndef	_CPU_PTE_H_
#define	_CPU_PTE_H_

#include <vm/vm_page.h>

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

	/* 64-bit PTE.  */
typedef	uint64_t	pt_entry_t;

	/* Number of PTEs in Level 1.  */
#define	NPTEL1		(PAGE_SIZE / sizeof (pt_entry_t))
#define	PTEL1SHIFT	(LOG2(PAGE_SIZE))

	/* Number of Level 1 pointers in Level 0.  */
#define	NL1PL0		(PAGE_SIZE / sizeof (struct pmap_lev1 *))
#define	L1L0SHIFT	(PTEL1SHIFT + LOG2(NPTEL1))

	/* Number of Level 0 pages required to map an entire address space.  */
#define	NL0PMAP		(128)
#define	PMAPL0SHIFT	(L1L0SHIFT + LOG2(NL1PL0))

struct pmap_lev1 {
	pt_entry_t pml1_entries[NPTEL1];
};

struct pmap_lev0 {
	struct pmap_lev1 *pml0_level1[NL1PL0];
};

struct pmap {
	struct pmap_lev0 *pm_level0[NL0PMAP];
	vaddr_t pm_base;
	vaddr_t pm_end;
	unsigned pm_asid;
};

/*
 * TLB and PTE management.  Most things operate within the context of
 * EntryLo0,1, and begin with TLBLO_.  Things which work with EntryHi
 * start with TLBHI_.  PTE bits begin with PG_.
 *
 * Note that while the TLB uses 4K pages, our PTEs correspond to VM pages,
 * which in turn are 8K.  This corresponds well to the fact that each TLB
 * entry maps 2 TLB pages (one even, one odd.)
 */
#define	TLB_PAGE_SHIFT	(PAGE_SHIFT - 1)
#define	TLB_PAGE_SIZE	(1 << TLB_PAGE_SHIFT)
#define	TLB_PAGE_MASK	(TLB_PAGE_SIZE - 1)

/*
 * TLB PageMask register.  Has mask bits set above the default, 4K, page mask.
 */
#define	TLBMASK_SHIFT	(13)
#define	TLBMASK_MASK	((PAGE_MASK >> TLBMASK_SHIFT) << TLBMASK_SHIFT)

/*
 * PFN for EntryLo register.  Upper bits are 0, which is to say that
 * bit 29 is the last hardware bit;  Bits 30 and upwards (EntryLo is
 * 64 bit though it can be referred to in 32-bits providing 2 software
 * bits safely.  We use it as 64 bits to get many software bits, and
 * god knows what else.) are unacknowledged by hardware.  They may be
 * written as anything, but otherwise they have as much meaning as
 * other 0 fields.
 */
#define	TLBLO_SWBITS_SHIFT	(30)
#define	TLBLO_PFN_SHIFT		(6 + (PAGE_SHIFT - TLBMASK_SHIFT))
#define	TLBLO_PFN_MASK		(0x03FFFFFC0)
#define	TLBLO_PA_TO_PFN(pa)	((((pa) >> (PAGE_SHIFT - 1)) << TLBLO_PFN_SHIFT) & TLBLO_PFN_MASK)
#define	TLBLO_PFN_TO_PA(pfn)	(((pfn) >> TLBLO_PFN_SHIFT) << (PAGE_SHIFT - 1))
#define	TLBLO_PTE_TO_PFN(pte)	((pte) & TLBLO_PFN_MASK)
#define	TLBLO_PTE_TO_PA(pte)	(TLBLO_PFN_TO_PA(TLBLO_PTE_TO_PFN((pte))))

/*
 * VPN for EntryHi register.  Upper two bits select user, supervisor,
 * or kernel.  Bits 61 to 40 copy bit 63.  VPN2 is bits 39 and down to
 * as low as 13, down to PAGE_SHIFT, to index 2 TLB pages*.  From bit 12
 * to bit 8 there is a 5-bit 0 field.  Low byte is ASID.
 *
 * Note that in Glatt, we map 2 TLB pages is equal to 1 VM page.
 */
#define	TLBHI_R_SHIFT		62
#define	TLBHI_R_USER		(0x00UL << TLBHI_R_SHIFT)
#define	TLBHI_R_SUPERVISOR	(0x01UL << TLBHI_R_SHIFT)
#define	TLBHI_R_KERNEL		(0x03UL << TLBHI_R_SHIFT)
#define	TLBHI_R_MASK		(0x03UL << TLBHI_R_SHIFT)
#define	TLBHI_VA_R(va)		((va) & TLBHI_R_MASK)
#define	TLBHI_FILL_SHIFT	48
#define	TLBHI_FILL_MASK		((0x7FFFFUL) << TLBHI_FILL_SHIFT)
#define	TLBHI_VA_FILL(va)	((((va) & (1UL << 63)) != 0 ? TLBHI_FILL_MASK : 0))
#define	TLBHI_VPN2_SHIFT	(PAGE_SHIFT)
#define	TLBHI_VPN2_MASK		(((~((1UL << TLBHI_VPN2_SHIFT) - 1)) << (63 - TLBHI_FILL_SHIFT)) >> (63 - TLBHI_FILL_SHIFT))
#define	TLBHI_VA_TO_VPN2(va)	((va) & TLBHI_VPN2_MASK)
#define	TLBHI_ENTRY(va, asid)	((TLBHI_VA_R((va))) /* Region. */ | \
				 (TLBHI_VA_FILL((va))) /* Fill. */ | \
				 (TLBHI_VA_TO_VPN2((va))) /* VPN2. */ | \
				 ((asid)))

/*
 * TLB flags managed in hardware:
 * 	C:	Cache attribute.
 * 	D:	Dirty bit.  This means a page is writable.  It is not
 * 		set at first, and a write is trapped, and the dirty
 * 		bit is set.  See also PG_RO.
 * 	V:	Valid bit.  Obvious, isn't it?
 * 	G:	Global bit.  This means that this mapping is present
 * 		in EVERY address space, and to ignore the ASID when
 * 		it is matched.
 */
#define	PG_C(attr)	((attr & 0x07) << 3)
#define	PG_C_UNCACHED	(PG_C(0x02))
#define	PG_C_CNC	(PG_C(0x03))
#define	PG_D		0x04
#define	PG_V		0x02
#define	PG_G		0x01

/*
 * VM flags managed in software:
 * 	RO:	Read only.  Never set PG_D on this page, and don't
 * 		listen to requests to write to it.
 */
#define	PG_RO	(0x01 << TLBLO_SWBITS_SHIFT)

/*
 * PTE management functions for bits defined above.
 */
#define	pte_clear(pte, bit)	atomic_clear_64((pte), (bit))
#define	pte_set(pte, bit)	atomic_set_64((pte), (bit))
#define	pte_test(pte, bit)	(((atomic_load_64((pte))) & (bit)) == (bit))

	/* Internal API for the MIPS PMAP.  */

unsigned pmap_asid(struct pmap *);
pt_entry_t *pmap_find(struct pmap *, vaddr_t);

#endif /* !_CPU_PTE_H_ */

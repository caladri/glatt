#ifndef	_PAGE_TABLE_H_
#define	_PAGE_TABLE_H_

#ifndef	ASSEMBLER
#include <vm/types.h>
#endif

/*
 * 64-bit PTE.
 */
#ifndef	ASSEMBLER
typedef	uint64_t	pt_entry_t;
#endif

/*
 * TLB and PTE management.  Most things operate within the context of
 * EntryLo0,1, and begin with TLBLO_.  Things which work with EntryHi
 * start with TLBHI_.  PTE bits begin with PG_.
 *
 * Note that while the TLB uses 4K pages, our PTEs correspond to VM pages,
 * which in turn are 8K.  This corresponds well to the fact that each TLB
 * entry maps 2 TLB pages (one even, one odd.)
 */

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
#define	TLBLO_PFN_SHIFT		(6)
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
#ifdef ASSEMBLER
#define	TLBHI_R_USER		(0x00 << TLBHI_R_SHIFT)
#define	TLBHI_R_SUPERVISOR	(0x01 << TLBHI_R_SHIFT)
#define	TLBHI_R_KERNEL		(0x03 << TLBHI_R_SHIFT)
#define	TLBHI_R_MASK		(0x03 << TLBHI_R_SHIFT)
#else
#define	TLBHI_R_USER		(0x00UL << TLBHI_R_SHIFT)
#define	TLBHI_R_SUPERVISOR	(0x01UL << TLBHI_R_SHIFT)
#define	TLBHI_R_KERNEL		(0x03UL << TLBHI_R_SHIFT)
#define	TLBHI_R_MASK		(0x03UL << TLBHI_R_SHIFT)
#endif
#define	TLBHI_VA_R(va)		((va) & TLBHI_R_MASK)
#define	TLBHI_FILL_SHIFT	40
#define	TLBHI_FILL_MASK		((0x7FFFFFUL) << TLBHI_FILL_SHIFT)
#define	TLBHI_VA_FILL(va)	((((va) & (1UL << 63)) != 0 ? TLBHI_FILL_MASK : 0))
#define	TLBHI_VPN2_SHIFT	(PAGE_SHIFT)
#ifdef ASSEMBLER
#define	TLBHI_VPN2_MASK		(((~((1 << TLBHI_VPN2_SHIFT) - 1)) << (63 - TLBHI_FILL_SHIFT)) >> (63 - TLBHI_FILL_SHIFT))
#else
#define	TLBHI_VPN2_MASK		(((~((1UL << TLBHI_VPN2_SHIFT) - 1)) << (63 - TLBHI_FILL_SHIFT)) >> (63 - TLBHI_FILL_SHIFT))
#endif
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
#define	PG_C_CCE	(PG_C(0x04))
#define	PG_C_CCEOW	(PG_C(0x05))
#define	PG_C_CCUOW	(PG_C(0x06))
#define	PG_D		0x04
#define	PG_V		0x02
#define	PG_G		0x01

/*
 * VM flags managed in software:
 * 	RO:	Read only.  Never set PG_D on this page, and don't
 * 		listen to requests to write to it.
 * 	W:	Wired.
 */
#define	PG_RO	(0x01 << TLBLO_SWBITS_SHIFT)
#define	PG_W	(0x02 << TLBLO_SWBITS_SHIFT)

/*
 * Amalgamations of flags in software:
 * 	PROT:	Protection related software flags.
 */
#define	PG_PROT	(PG_RO)

/*
 * PTE management functions for bits defined above.
 */
#define	pte_clear(pte, bit)	((*(pte)) &= ~(bit))
#define	pte_set(pte, bit)	((*(pte)) |= (bit))
#define	pte_test(pte, bit)	(((*(pte)) & (bit)) == (bit))

#ifndef	ASSEMBLER
pt_entry_t *pmap_find(struct vm *, vaddr_t);
#endif

#endif /* !_PAGE_TABLE_H_ */

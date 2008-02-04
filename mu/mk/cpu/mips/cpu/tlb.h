#ifndef	_CPU_TLB_H_
#define	_CPU_TLB_H_

#include <vm/types.h>

struct pmap;

/*
 * XXX since we are using 4K TLB pages and 8K VM pages (2 for the price of 1!),
 * we must always make sure to mask off the PAGE_MASK bits on pages that come
 * from VM, as we have to manage the highest bit set in PAGE_MASK, and if it
 * has it set with random garbage, we're screwed.
 *
 * So when a TLB refill routine is written, we should double-check that, or the
 * code which manages the PTEs must be really careful (which it should be in
 * any event.)
 */

#define	TLB_PAGE_SIZE	(PAGE_SIZE / 2)

/*
 * Wired entries in TLB context start at entry 1 and there can be at most 8 of
 * them.  Entry 0 is reserved for PCPU.
 */
#define	TLB_WIRED_PCPU	(0)
#define	TLB_WIRED_BASE	(1)
#define	TLB_WIRED_COUNT	(8)

struct tlb_wired_entry {
	register_t twe_entryhi;
	register_t twe_entrylo0;
	register_t twe_entrylo1;
};

struct tlb_wired_context {
	struct tlb_wired_entry twc_entries[TLB_WIRED_COUNT];
};

	/* An interface to the TLB.  */
void tlb_init(struct pmap *, paddr_t);
void tlb_invalidate(struct pmap *, vaddr_t);
void tlb_modify(vaddr_t);
void tlb_refill(vaddr_t);
void tlb_update(struct pmap *, vaddr_t);

	/* An interface for managing wired TLB entries.  */
void tlb_wired_init(struct tlb_wired_context *);
void tlb_wired_wire(struct tlb_wired_context *, struct pmap *, vaddr_t);

#endif /* !_CPU_TLB_H_ */

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

	/* An interface to the TLB.  */
void tlb_init(paddr_t);
void tlb_invalidate(struct pmap *, vaddr_t);
void tlb_modify(vaddr_t);
void tlb_refill(vaddr_t);
void tlb_update(struct pmap *, vaddr_t);

#endif /* !_CPU_TLB_H_ */

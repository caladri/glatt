#ifndef	_CPU_TLB_H_
#define	_CPU_TLB_H_

#include <vm/types.h>

struct pmap;

	/* An interface to the TLB.  */
void tlb_init(paddr_t);
void tlb_invalidate(struct pmap *, vaddr_t);
void tlb_modify(vaddr_t);
void tlb_refill(vaddr_t);
void tlb_update(struct pmap *, vaddr_t);

#endif /* !_CPU_TLB_H_ */

#ifndef	_CPU_TLB_H_
#define	_CPU_TLB_H_

#include <vm/types.h>

	/* An interface to the TLB.  */
void tlb_init(paddr_t);
void tlb_invalidate(struct vm *, vaddr_t);
void tlb_update(struct vm *, vaddr_t);

#endif /* !_CPU_TLB_H_ */

#ifndef	_CPU_TLB_H_
#define	_CPU_TLB_H_

#include <vm/types.h>

	/* An interface to the TLB.  */
void tlb_init(paddr_t);
void tlb_invalidate(vaddr_t, unsigned);

#endif /* !_CPU_TLB_H_ */

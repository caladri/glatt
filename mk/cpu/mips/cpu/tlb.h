#ifndef	_CPU_TLB_H_
#define	_CPU_TLB_H_

struct pmap;

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
void tlb_wired_load(struct tlb_wired_context *);
void tlb_wired_wire(struct tlb_wired_context *, struct pmap *, vaddr_t);

#endif /* !_CPU_TLB_H_ */

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
void tlb_init(paddr_t, unsigned);
void tlb_invalidate(unsigned, vaddr_t);
void tlb_modify(vaddr_t);

	/* An interface for managing wired TLB entries.  */
void tlb_wired_init(struct tlb_wired_context *) __non_null(1);
void tlb_wired_load(struct tlb_wired_context *) __non_null(1);
void tlb_wired_wire(struct tlb_wired_context *, struct pmap *, vaddr_t) __non_null(1, 2);

#endif /* !_CPU_TLB_H_ */

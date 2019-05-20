#include <core/types.h>
#include <core/critical.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/memory.h>
#include <cpu/mp.h>
#include <cpu/pcpu.h>
#include <cpu/pmap.h>
#include <cpu/pte.h>
#include <cpu/tlb.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <core/console.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

/*
 * PageMask must increment in steps of 2 bits.
 */
COMPILE_TIME_ASSERT(POPCNT(TLBMASK_MASK) % 2 == 0);

#ifndef	UNIPROCESSOR
struct tlb_shootdown_arg {
	struct pmap *pmap;
	vaddr_t vaddr;
};
#endif

static inline void
tlb_probe(void)
{
	asm volatile ("tlbp" : : : "memory");
	cpu_barrier();
}

static inline void
tlb_read(void)
{
	asm volatile ("tlbr" : : : "memory");
	cpu_barrier();
}

static inline void
tlb_write_indexed(void)
{
	asm volatile ("tlbwi" : : : "memory");
	cpu_barrier();
}

static inline void
tlb_write_random(void)
{
	asm volatile ("tlbwr" : : : "memory");
	cpu_barrier();
}

static void tlb_invalidate_addr(struct pmap *, vaddr_t);
static void tlb_invalidate_one(unsigned);
#ifndef	UNIPROCESSOR
static void tlb_shootdown(void *);
#endif
static void tlb_update(struct pmap *, vaddr_t, pt_entry_t);
static void tlb_wired_entry(struct tlb_wired_entry *, vaddr_t, unsigned, pt_entry_t);
static void tlb_wired_insert(unsigned, struct tlb_wired_entry *);

void
tlb_init(paddr_t pcpu_addr, unsigned ntlbs)
{
	struct tlb_wired_entry twe;
	unsigned i;

	ASSERT(PAGE_ALIGNED(pcpu_addr), "PCPU address must be page-aligned.");

	/* Invalidate all entries.  */
	for (i = 0; i < ntlbs; i++)
		tlb_invalidate_one(i);

	/* Set address-space ID to the kernel's ASID.  */
	cpu_write_tlb_entryhi(PMAP_ASID_RESERVED);

	/* Don't keep any old wired entries.  */
	cpu_write_tlb_wired(0);

	/* Set the pagemask.  */
	cpu_write_tlb_pagemask(TLBMASK_MASK);

	/*
	 * Insert a wired mapping for the per-CPU data at the start of the
	 * address space.  Can't invalidate until we do this, as we need the
	 * PCPU data to get the number of TLB entries, which needs to be
	 * mapped to work.
	 */
	tlb_wired_entry(&twe, PCPU_VIRTUAL, PMAP_ASID_RESERVED,
			TLBLO_PA_TO_PFN(pcpu_addr) | PG_V | PG_D | PG_G | PG_C_CNC);
	tlb_wired_insert(TLB_WIRED_PCPU, &twe);
}

void
tlb_invalidate(struct pmap *pm, vaddr_t vaddr)
{
	tlb_invalidate_addr(pm, vaddr);

#ifndef	UNIPROCESSOR
	/*
	 * XXX
	 * Check if this VA is active on any other CPUs.
	 */
	if (mp_ncpus() != 1) {
		struct tlb_shootdown_arg shootdown;

		shootdown.pmap = pm;
		shootdown.vaddr = vaddr;

		mp_hokusai_origin(NULL, NULL,
				  tlb_shootdown, &shootdown);
	}
#endif
}

void
tlb_modify(vaddr_t vaddr)
{
	pt_entry_t *pte;
	struct vm *vm;

	if (PAGE_FLOOR(vaddr) == 0)
		panic("%s: accessing NULL.", __func__);
	if (vaddr >= KERNEL_BASE && vaddr < KERNEL_END)
		vm = &kernel_vm;
	else
		vm = current_task()->t_vm;
	pte = pmap_find(vm->vm_pmap, vaddr); /* XXX lock.  */
	if (pte == NULL)
		panic("%s: pmap_find returned NULL.", __func__);
	if (pte_test(pte, PG_RO))
		panic("%s: write to read-only page.", __func__);
	if (pte_test(pte, PG_D))
		panic("%s: modifying already dirty page.", __func__);
	pte_set(pte, PG_D);	/* Mark page dirty.  */
	tlb_update(vm->vm_pmap, vaddr, *pte);
}

void
tlb_wired_init(struct tlb_wired_context *wired)
{
	struct tlb_wired_entry *twe;
	unsigned i;

	for (i = 0; i < TLB_WIRED_COUNT; i++) {
		twe = &wired->twc_entries[i];
		twe->twe_entryhi = 0;
	}
}

void
tlb_wired_wire(struct tlb_wired_context *wired, struct pmap *pm, vaddr_t vaddr)
{
	struct tlb_wired_entry *twe;
	pt_entry_t *pte;
	unsigned i;

	vaddr &= ~PAGE_MASK;

	for (i = 0; i < TLB_WIRED_COUNT; i++) {
		twe = &wired->twc_entries[i];
		if (twe->twe_entryhi == 0)
			break;
		twe = NULL;
	}
	if (twe == NULL)
		panic("%s: no free wired slots.", __func__);

	pte = pmap_find(pm, vaddr); /* XXX lock.  */
	if (pte == NULL)
		panic("%s: pmap_find returned NULL.", __func__);
	pte_set(pte, PG_D);	/* XXX Mark page dirty.  */
	tlb_invalidate_addr(pm, vaddr);
	tlb_wired_entry(twe, vaddr, pmap_asid(pm), *pte);
}

static void
tlb_invalidate_addr(struct pmap *pm, vaddr_t vaddr)
{
	register_t asid;
	int i;

	vaddr &= ~PAGE_MASK;

	critical_enter();
	asid = cpu_read_tlb_entryhi() & TLBHI_ASID_MASK;
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, pmap_asid(pm)));
	tlb_probe();
	i = cpu_read_tlb_index();
	if (i >= 0)
		tlb_invalidate_one(i);
	cpu_write_tlb_entryhi(asid);
	critical_exit();
}

static void
tlb_invalidate_one(unsigned i)
{
	/*
	 * ASID is saved and restored by caller, no need to do it here.
	 */
	cpu_write_tlb_entryhi(TLBHI_ENTRY(KSEG2_BASE + (i * PAGE_SIZE), 0));
	cpu_write_tlb_entrylo0(0);
	cpu_write_tlb_entrylo1(0);
	cpu_write_tlb_index(i);
	tlb_write_indexed();
}

#ifndef	UNIPROCESSOR
static void
tlb_shootdown(void *arg)
{
	struct tlb_shootdown_arg *tsa;

	tsa = arg;
	tlb_invalidate_addr(tsa->pmap, tsa->vaddr);
}
#endif

static void
tlb_update(struct pmap *pm, vaddr_t vaddr, pt_entry_t pte)
{
	register_t asid;
	int i;

	critical_enter();
	asid = cpu_read_tlb_entryhi() & TLBHI_ASID_MASK;
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, pmap_asid(pm)));
	tlb_probe();
	i = cpu_read_tlb_index();
	if (i >= 0) {
		cpu_write_tlb_entrylo0(pte);
		cpu_write_tlb_entrylo1(pte + TLBLO_PA_TO_PFN(TLB_PAGE_SIZE));

		tlb_write_indexed();
	}
	cpu_write_tlb_entryhi(asid);
	critical_exit();
}

static void
tlb_wired_entry(struct tlb_wired_entry *twe, vaddr_t vaddr, unsigned asid,
		pt_entry_t pte)
{
	twe->twe_entryhi = TLBHI_ENTRY(vaddr, asid);
	twe->twe_entrylo0 = pte;
	twe->twe_entrylo1 = pte + TLBLO_PA_TO_PFN(TLB_PAGE_SIZE);
}

static void
tlb_wired_insert(unsigned index, struct tlb_wired_entry *twe)
{
	ASSERT(startup_early, "Must be serialized for startup.");

	cpu_write_tlb_entryhi(twe->twe_entryhi);
	cpu_write_tlb_entrylo0(twe->twe_entrylo0);
	cpu_write_tlb_entrylo1(twe->twe_entrylo1);
	cpu_write_tlb_index(index);
	tlb_write_indexed();
	if (index + 1 > cpu_read_tlb_wired())
		cpu_write_tlb_wired(index + 1);
}

#ifdef DB
static void
db_cpu_dump_tlb_lo(unsigned n, register_t lo)
{
	printf(" Lo%u\t%lx\tpa %lx\tcache attribute %lx\t%s\t%s\t%s\n", n,
		 lo, TLBLO_PTE_TO_PA(lo), lo & PG_C(~0),
		 (lo & PG_D) != 0 ? "dirty" : "clean",
		 (lo & PG_V) != 0 ? "valid" : "invalid",
		 (lo & PG_G) != 0 ? "global" : "local");
}

static void
db_cpu_dump_tlb(void)
{
	register_t ehi, elo0, elo1, pmask;
	unsigned i;

	printf("Beginning TLB dump...\n");
	for (i = 0; i < PCPU_GET(cpuinfo).cpu_ntlbs; i++) {
		if (i == cpu_read_tlb_wired()) {
			if (i != 0)
				printf("^^^ WIRED ENTRIES ^^^\n");
			else
				printf("(No wired entries.)\n");
		}
		cpu_write_tlb_index(i);
		tlb_read();

		ehi = cpu_read_tlb_entryhi();
		elo0 = cpu_read_tlb_entrylo0();
		elo1 = cpu_read_tlb_entrylo1();
		pmask = cpu_read_tlb_pagemask();

		if (elo0 == 0 && elo1 == 0)
			continue;

		printf("#%u\t=> %lx region %lx vpn2 %lx pagemask %lx", i, ehi, (ehi & TLBHI_R_MASK) >> TLBHI_R_SHIFT, ehi & TLBHI_VPN2_MASK, pmask);
		if ((elo0 & PG_G) != 0 && (elo1 & PG_G) != 0)
			printf(" global\n");
		else
			printf(" asid %u\n", (unsigned)ehi & 0xff);
		db_cpu_dump_tlb_lo(0, elo0);
		db_cpu_dump_tlb_lo(1, elo1);
	}
	printf("Finished.\n");
}
DB_COMMAND(tlb, cpu, db_cpu_dump_tlb);
#endif

#include <core/types.h>
#include <core/critical.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/pte.h>
#include <cpu/tlb.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <io/console/console.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

/*
 * PageMask must increment in steps of 2 bits.
 */
COMPILE_TIME_ASSERT(POPCNT(TLBMASK_MASK) % 2 == 0);

#ifndef	UNIPROCESSOR
struct tlb_shootdown_arg {
	unsigned asid;
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

static void tlb_invalidate_addr(unsigned, vaddr_t);
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

	/* XXX Set address-space ID to the kernel's ASID.  */
	cpu_write_tlb_entryhi(0);

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
	tlb_wired_entry(&twe, PCPU_VIRTUAL, 0,
			TLBLO_PA_TO_PFN(pcpu_addr) | PG_V | PG_D | PG_G);
	tlb_wired_insert(TLB_WIRED_PCPU, &twe);
}

void
tlb_invalidate(unsigned asid, vaddr_t vaddr)
{
	tlb_invalidate_addr(asid, vaddr);

#ifndef	UNIPROCESSOR
	/*
	 * XXX
	 * Check if this VA is active on any other CPUs.
	 */
	if (mp_ncpus() != 1) {
		struct tlb_shootdown_arg shootdown;

		shootdown.asid = asid;
		shootdown.vaddr = vaddr;

		mp_hokusai_master(NULL, NULL,
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
	tlb_invalidate_addr(pmap_asid(pm), vaddr);
	tlb_wired_entry(twe, vaddr, pmap_asid(pm), *pte);
}

static void
tlb_invalidate_addr(unsigned asid, vaddr_t vaddr)
{
	int i;

	vaddr &= ~PAGE_MASK;

	critical_enter();
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, asid));
	tlb_probe();
	i = cpu_read_tlb_index();
	if (i >= 0)
		tlb_invalidate_one(i);
	critical_exit();
}

static void
tlb_invalidate_one(unsigned i)
{
	/* XXX an invalid ASID? */
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
	tlb_invalidate_addr(tsa->asid, tsa->vaddr);
}
#endif

static void
tlb_update(struct pmap *pm, vaddr_t vaddr, pt_entry_t pte)
{
	register_t asid;
	int i;

	critical_enter();
	asid = cpu_read_tlb_entryhi();
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, pmap_asid(pm)));
	tlb_probe();
	i = cpu_read_tlb_index();
	cpu_write_tlb_entrylo0(pte);
	cpu_write_tlb_entrylo1(pte + TLBLO_PA_TO_PFN(TLB_PAGE_SIZE));
	if (i >= 0)
		tlb_write_indexed();
	else
		tlb_write_random();
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
db_cpu_dump_tlb(void)
{
	register_t ehi, elo0, elo1;
	unsigned i;

	kcprintf("Beginning TLB dump...\n");
	for (i = 0; i < PCPU_GET(cpuinfo).cpu_ntlbs; i++) {
		if (i == cpu_read_tlb_wired()) {
			if (i != 0)
				kcprintf("^^^ WIRED ENTRIES ^^^\n");
			else
				kcprintf("(No wired entries.)\n");
		}
		cpu_write_tlb_index(i);
		tlb_read();

		ehi = cpu_read_tlb_entryhi();
		elo0 = cpu_read_tlb_entrylo0();
		elo1 = cpu_read_tlb_entrylo1();

		if (elo0 == 0 && elo1 == 0)
			continue;

		kcprintf("#%u\t=> %lx\n", i, ehi);
		kcprintf(" Lo0\t%lx\n", elo0);
		kcprintf(" Lo1\t%lx\n", elo1);
	}
	kcprintf("Finished.\n");
}
DB_COMMAND(tlb, cpu, db_cpu_dump_tlb);
#endif

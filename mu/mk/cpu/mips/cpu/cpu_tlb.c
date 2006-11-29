#include <core/types.h>
#include <core/critical.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/tlb.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <page/page_map.h>
#include <page/page_table.h>
#include <vm/page.h>
#include <vm/vm.h>

/*
 * Right now this code (and hopefully only this code) assumes that we are using
 * 8K pages in VM.  If we are, then we can use the default TLB pagesize (4K)
 * and just map the first and the second consecutive 4K pages.  If anything
 * outside of here breaks, that's bad.
 */
COMPILE_TIME_ASSERT(PAGE_SIZE == 8192);
COMPILE_TIME_ASSERT(TLB_PAGE_SIZE == 4096);

static __inline void
tlb_probe(void)
{
	__asm __volatile ("tlbp" : : : "memory");
	cpu_barrier();
}

static __inline void
tlb_read(void)
{
	__asm __volatile ("tlbr" : : : "memory");
	cpu_barrier();
}

static __inline void
tlb_write_indexed(void)
{
	__asm __volatile ("tlbwi" : : : "memory");
	cpu_barrier();
}

static __inline void
tlb_write_random(void)
{
	__asm __volatile ("tlbwr" : : : "memory");
	cpu_barrier();
}

static void tlb_insert_wired(struct pmap *pm, vaddr_t, paddr_t);
static void tlb_invalidate_all(void);
static void tlb_invalidate_one(unsigned);

void
tlb_init(struct pmap *pm, paddr_t pcpu_addr)
{
	critical_section_t crit;

	crit = critical_enter();

	/* Set address-space ID to the kernel's ASID.  */
	cpu_write_tlb_entryhi(pmap_asid(pm));

	/* Don't keep any old wired entries.  */
	cpu_write_tlb_wired(0);

	/* Set the pagemask.  */
	cpu_write_tlb_pagemask(0);	/* XXX depends on TLB_PAGE_SIZE.  */

	/*
	 * Insert a wired mapping for the per-CPU data at the start of the
	 * address space.  Can't invalidate until we do this, as we need the
	 * PCPU data to get the number of TLB entries, which needs to be
	 * mapped to work.
	 */
	tlb_insert_wired(pm, PCPU_VIRTUAL, pcpu_addr);

	/* Invalidate all entries (except wired ones.)  */
	tlb_invalidate_all();

	critical_exit(crit);
}

void
tlb_invalidate(struct pmap *pm, vaddr_t vaddr)
{
	critical_section_t crit;
	int i;

	vaddr &= ~PAGE_MASK;

	crit = critical_enter();
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, pmap_asid(pm)));
	tlb_probe();
	i = cpu_read_tlb_index();
	if (i >= 0)
		tlb_invalidate_one(i);
	critical_exit(crit);
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
		vm = current_thread()->td_parent->t_vm;
	pte = pmap_find(vm->vm_pmap, vaddr); /* XXX lock.  */
	if (pte == NULL)
		panic("%s: pmap_find returned NULL.", __func__);
	if (pte_test(pte, PG_RO))
		panic("%s: write to read-only page.", __func__);
	pte_set(pte, PG_D);	/* Mark page dirty.  */
	tlb_update(vm->vm_pmap, vaddr);
}

void
tlb_refill(vaddr_t vaddr)
{
	struct vm *vm;

	if (PAGE_FLOOR(vaddr) == 0)
		panic("%s: accessing NULL.", __func__);
	if (vaddr >= KERNEL_BASE && vaddr < KERNEL_END)
		vm = &kernel_vm;
	else
		vm = current_thread()->td_parent->t_vm;
	tlb_update(vm->vm_pmap, vaddr);
}

void
tlb_update(struct pmap *pm, vaddr_t vaddr)
{
	critical_section_t crit;
	register_t asid;
	pt_entry_t *pte;
	int i;

	crit = critical_enter();
	pte = pmap_find(pm, vaddr); /* XXX lock.  */
	if (pte == NULL)
		panic("%s: pmap_find returned NULL.", __func__);
	asid = cpu_read_tlb_entryhi();
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, pmap_asid(pm)));
	tlb_probe();
	i = cpu_read_tlb_index();
	cpu_write_tlb_entrylo0(*pte);
	cpu_write_tlb_entrylo1(*pte + TLBLO_PA_TO_PFN(TLB_PAGE_SIZE));
	if (i >= 0)
		tlb_write_indexed();
	else
		tlb_write_random();
	cpu_write_tlb_entryhi(asid);
	critical_exit(crit);
}

static void
tlb_insert_wired(struct pmap *pm, vaddr_t vaddr, paddr_t paddr)
{
	critical_section_t crit;

	vaddr &= ~PAGE_MASK;
	paddr &= ~PAGE_MASK;

	crit = critical_enter();
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, pmap_asid(pm)));
	cpu_write_tlb_entrylo0(TLBLO_PA_TO_PFN(paddr) |
			       PG_V | PG_D | PG_G | PG_C_CNC);
	cpu_write_tlb_entrylo1(TLBLO_PA_TO_PFN(paddr + TLB_PAGE_SIZE) |
			       PG_V | PG_D | PG_G | PG_C_CNC);
	cpu_write_tlb_index(cpu_read_tlb_wired());
	tlb_write_indexed();
	cpu_write_tlb_wired(cpu_read_tlb_wired() + 1);
	critical_exit(crit);
}

/*
 * Note that this skips wired entries.
 */
static void
tlb_invalidate_all(void)
{
	critical_section_t crit;
	register_t asid;
	unsigned i;

	crit = critical_enter();
	asid = cpu_read_tlb_entryhi();
	for (i = cpu_read_tlb_wired(); i < PCPU_GET(cpuinfo).cpu_ntlbs; i++)
		tlb_invalidate_one(i);
	cpu_write_tlb_entryhi(asid);
	critical_exit(crit);
}

static void
tlb_invalidate_one(unsigned i)
{
	/* XXX an invalid ASID? */
	cpu_write_tlb_entryhi(TLBHI_ENTRY(XKPHYS_BASE + (i * PAGE_SIZE), 0));
	cpu_write_tlb_entrylo0(0);
	cpu_write_tlb_entrylo1(0);
	cpu_write_tlb_index(i);
	tlb_write_indexed();
}

#if 0
static void
tlb_db_dump(void)
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
DB_COMMAND(tlb_dump, tlb_db_dump, "Dump contents of the TLB.");
#endif

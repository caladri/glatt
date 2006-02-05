#include <core/types.h>
#include <core/critical.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/tlb.h>
#include <page/page_table.h>
#include <vm/page.h>

#define	TLB_PAGE_SIZE	(PAGE_SIZE / 2)

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

static void tlb_insert_wired(vaddr_t, paddr_t);
static void tlb_invalidate_all(void);
static void tlb_invalidate_one(unsigned);

void
tlb_init(paddr_t pcpu_addr)
{
	critical_section_t crit;

	crit = critical_enter();

	/* Set address-space ID to zero.  */
	cpu_write_tlb_entryhi(0);

	/* Don't keep any old wired entries.  */
	cpu_write_tlb_wired(0);

	/* Invalidate all entries.  */
	tlb_invalidate_all();

	/* Set the pagemask.  */
	cpu_write_tlb_pagemask(0);	/* XXX depends on TLB_PAGE_SIZE.  */

	critical_exit(crit);

	/*
	 * Insert a wired mapping for the per-CPU data at the start of the
	 * address space.
	 */
	tlb_insert_wired(PCPU_VIRTUAL, pcpu_addr);
}

static void
tlb_insert_wired(vaddr_t vaddr, paddr_t paddr)
{
	critical_section_t crit;

	vaddr &= ~PAGE_MASK;
	paddr &= ~PAGE_MASK;

	crit = critical_enter();
	cpu_write_tlb_entryhi(TLBHI_ENTRY(vaddr, 0));
	cpu_write_tlb_entrylo0(TLBLO_PA_TO_PFN(paddr) |
			       PG_V | PG_D | PG_G | PG_C_CNC);
	cpu_write_tlb_entrylo1(TLBLO_PA_TO_PFN(paddr + TLB_PAGE_SIZE) |
			       PG_V | PG_D | PG_G | PG_C_CNC);
	cpu_write_tlb_index(cpu_read_tlb_wired());
	tlb_write_indexed();
	cpu_write_tlb_wired(cpu_read_tlb_wired() + 1);
	critical_exit(crit);
}

static void
tlb_invalidate_all(void)
{
	critical_section_t crit;
	register_t asid;
	unsigned i;

	crit = critical_enter();
	asid = cpu_read_tlb_entryhi();
	for (i = 0; i < pcpu_me()->pc_cpuinfo.cpu_ntlbs; i++) {
		tlb_invalidate_one(i);
	}
	cpu_write_tlb_entryhi(asid);
	critical_exit(crit);
}

static void
tlb_invalidate_one(unsigned i)
{
	cpu_write_tlb_entryhi(TLBHI_ENTRY(XKPHYS_BASE + (i * PAGE_SIZE), 0));
	cpu_write_tlb_entrylo0(0);
	cpu_write_tlb_entrylo1(0);
	cpu_write_tlb_index(i);
	tlb_write_indexed();
}

#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/pmap.h>
#include <cpu/pte.h>
#include <cpu/tlb.h>
#ifdef DB
#include <db/db_command.h>
#endif
#if defined(DB) || defined(VERBOSE)
#include <core/console.h>
#endif
#include <vm/vm.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

#ifdef DB
DB_COMMAND_TREE_DECLARE(pmap);
DB_COMMAND_TREE(pmap, cpu, pmap);
#endif

/*
 * Make sure our page structures are correctly-sized.
 */
COMPILE_TIME_ASSERT(sizeof (struct pmap_lev1) == PAGE_SIZE);
COMPILE_TIME_ASSERT(sizeof (struct pmap_lev0) == PAGE_SIZE);
/*
 * If it goes over PAGE_SIZE, we would need a way to allocate a contiguous
 * direct-mapped region, or we would have to have the page tables self-map the
 * pmap entry.
 */
COMPILE_TIME_ASSERT(sizeof (struct pmap) <= PAGE_SIZE);

/*
 * Page-table indexing inlines.
 */
static inline vaddr_t
pmap_index_pte(vaddr_t vaddr)
{
	return ((vaddr >> PTEL1SHIFT) % NPTEL1);
}

static inline vaddr_t
pmap_index1(vaddr_t vaddr)
{
	return ((vaddr >> L1L0SHIFT) % NL1PL0);
}

static inline vaddr_t
pmap_index0(vaddr_t vaddr)
{
	return ((vaddr >> PMAPL0SHIFT) % NL0PMAP);
}

static void pmap_alloc_asid(struct pmap *);
static int pmap_alloc_pte(struct pmap *, vaddr_t, pt_entry_t **);
static void pmap_collect(struct pmap *);
static struct pmap_lev0 *pmap_find0(struct pmap *, vaddr_t);
static struct pmap_lev1 *pmap_find1(struct pmap_lev0 *, vaddr_t);
static pt_entry_t *pmap_find_pte(struct pmap_lev1 *, vaddr_t);
static bool pmap_is_direct(vaddr_t);
static void pmap_pinit(struct pmap *, vaddr_t, vaddr_t);
static void pmap_update(struct pmap *, vaddr_t, struct vm_page *, pt_entry_t);

static struct pool pmap_pool;

unsigned
pmap_asid(struct pmap *pm)
{
	ASSERT(pm != NULL, "cannot get ASID for NULL pmap");
#ifdef UNIPROCESSOR
	return (pm->pm_asid);
#else
	return (pm->pm_asid[mp_whoami()]);
#endif
}

void
pmap_activate(struct vm *vm)
{
	ASSERT(vm != NULL, "cannot activate pmap for NULL VM");

	/*
	 * XXX
	 * If need be, allocate an ASID.
	 *
	 * This needs to happen if we have never run on this CPU.
	 *
	 * This also needs to happen if the ASID we have is from a prior
	 * generation.
	 */
	if (!cpu_bitmask_is_set(&vm->vm_pmap->pm_active, mp_whoami())) {
		pmap_alloc_asid(vm->vm_pmap);
		cpu_bitmask_set(&vm->vm_pmap->pm_active, mp_whoami());
	}
	cpu_write_tlb_entryhi(pmap_asid(vm->vm_pmap));
}

void
pmap_bootstrap(void)
{
	int error;

#ifdef VERBOSE_DEBUG
	printf("PMAP: %u level 0 pointers in each pmap.\n", NL0PMAP);
	printf("PMAP: %lu level 1 pointers in each level 0 page.\n", NL1PL0);
	printf("PMAP: %lu PTEs in each level 1 page.\n", NPTEL1);
	printf("PMAP: Level 1 maps %lu pages of virtual address space.\n",
		 NPTEL1);
	printf("PMAP: Level 0 maps %lu pages of virtual address space.\n",
		 NPTEL1 * NL1PL0);
	printf("PMAP: Each pmap maps %lu pages of virtual address space.\n",
		 NPTEL1 * NL1PL0 * NL0PMAP);
	printf("PMAP: Each pmap maps %luM (%luG) of virtual address space.\n",
		 ((NPTEL1 * NL1PL0 * NL0PMAP * PAGE_SIZE) /
		  (1024 * 1024)),
		 ((NPTEL1 * NL1PL0 * NL0PMAP * PAGE_SIZE) /
		  (1024 * 1024 * 1024)));
#endif

	error = pool_create(&pmap_pool, "PMAP", sizeof (struct pmap),
			    POOL_DEFAULT);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	error = pmap_init(&kernel_vm, KERNEL_BASE, KERNEL_END);
	if (error != 0)
		panic("%s: pmap_init failed: %m", __func__, error);
}

int
pmap_extract(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
	struct pmap *pm;
	pt_entry_t *pte;

	pm = vm->vm_pmap;

	vaddr &= ~PAGE_MASK;

	if (pmap_is_direct(vaddr)) {
		if (vm != &kernel_vm)
			return (ERROR_NOT_PERMITTED);
		*paddrp = XKPHYS_EXTRACT(vaddr);
		return (0);
	}

	if (vaddr >= pm->pm_end || vaddr < pm->pm_base)
		return (ERROR_NOT_PERMITTED);
	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		return (ERROR_NOT_FOUND);
	if (!pte_test(pte, PG_V))
		return (ERROR_NOT_FOUND);
	*paddrp = TLBLO_PTE_TO_PA(*pte);
	return (0);
}

pt_entry_t *
pmap_find(struct pmap *pm, vaddr_t vaddr)
{
	struct pmap_lev0 *pml0;
	struct pmap_lev1 *pml1;

	if (vaddr < pm->pm_base || vaddr >= pm->pm_end)
		return (NULL);

	pml0 = pmap_find0(pm, vaddr);
	if (pml0 == NULL)
		return (NULL);
	pml1 = pmap_find1(pml0, vaddr);
	if (pml1 == NULL)
		return (NULL);
	return (pmap_find_pte(pml1, vaddr));
}

int
pmap_init(struct vm *vm, vaddr_t base, vaddr_t end)
{
	struct pmap *pm;
	int error;

	pm = pool_allocate(&pmap_pool);
	if (pm == NULL)
		return (ERROR_EXHAUSTED);
	vm->vm_pmap = pm;
	pmap_pinit(vm->vm_pmap, base, end);

	/*
	 * Skip the first page of any mapping.  For userland, that's NULL.  For
	 * kernel, that's where the PCPU data goes.
	 */
	error = vm_insert_range(vm, base + PAGE_SIZE, end);
	if (error != 0) {
		pool_free(pm);
		return (error);
	}
	return (0);
}

int
pmap_map(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	struct pmap *pm;
	pt_entry_t *pte, flags;
	int error;

	pm = vm->vm_pmap;

	if (vaddr >= pm->pm_end || vaddr < pm->pm_base)
		return (ERROR_NOT_PERMITTED);

	error = pmap_alloc_pte(pm, vaddr, &pte);
	if (error != 0) {
		pmap_collect(pm);
		return (error);
	}
	flags = PG_V;
	if (vm == &kernel_vm)
		flags |= PG_G;
	flags |= PG_C_CNC;
	pmap_update(pm, vaddr, page, flags);
	return (0);
}

int
pmap_unmap(struct vm *vm, vaddr_t vaddr)
{
	struct pmap *pm;
	pt_entry_t *pte;

	pm = vm->vm_pmap;

	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		return (ERROR_NOT_FOUND);
	if (!pte_test(pte, PG_V))
		return (0);
	/* Invalidate by updating to not have PG_V set.  */
	tlb_invalidate(pm, vaddr);
	atomic_store64(pte, 0);
	return (0);
}

void
pmap_zero(struct vm_page *page)
{
	paddr_t paddr;
	uint64_t *p;
	size_t i;

	paddr = page_address(page);
	p = (uint64_t *)XKPHYS_MAP(CCA_CNC, paddr);

	for (i = 0; i < (PAGE_SIZE / sizeof *p) / 16; i++) {
		p[0x0] = p[0x1] = p[0x2] = p[0x3] =
		p[0x4] = p[0x5] = p[0x6] = p[0x7] =
		p[0x8] = p[0x9] = p[0xa] = p[0xb] =
		p[0xc] = p[0xd] = p[0xe] = p[0xf] = 0x0;
		p += 16;
	}
}

static void
pmap_alloc_asid(struct pmap *pm)
{
	unsigned asid;
	cpu_id_t cpu;

	if (pm == kernel_vm.vm_pmap) {
#ifdef UNIPROCESSOR
		pm->pm_asid = PMAP_ASID_RESERVED;
#else
		for (cpu = 0; cpu < MAXCPUS; cpu++)
			pm->pm_asid[cpu] = PMAP_ASID_RESERVED;
#endif
		return;
	}
	/*
	 * XXX
	 * If we use ASID generations and the generation is the same as the
	 * current one, just reuse it rather than generating a new one and
	 * possibly forcing a TLB flush.
	 */
	ASSERT(critical_section(), "Must already be in a critical section.");
	critical_enter();
	asid = PCPU_GET(asidnext);
	if (asid == PMAP_ASID_MAX) {
		PCPU_SET(asidnext, PMAP_ASID_FIRST);
		/* XXX Do something with an asid generation like *BSD? */
		/* XXX Flush TLB.  ASIDs may be reused.  */
		printf("%s: ASID wrap\n", __func__);
	} else {
		PCPU_SET(asidnext, asid + 1);
	}
	critical_exit();

#ifdef UNIPROCESSOR
	pm->pm_asid = asid;
#else
	pm->pm_asid[mp_whoami()] = asid;
#endif
}

static int
pmap_alloc_pte(struct pmap *pm, vaddr_t vaddr, pt_entry_t **ptep)
{
	struct pmap_lev0 *pml0;
	struct pmap_lev1 *pml1;
	unsigned pml0i, pml1i;
	vaddr_t tmpaddr;
	int error;

	/* We have to have a pmap before getting here.  */
	if (pm == NULL)
		panic("%s: allocating into NULL pmap.", __func__);

	pml0i = pmap_index0(vaddr);
	if (pm->pm_level0[pml0i] == NULL) {
		error = page_alloc_direct(&kernel_vm,
					  PAGE_FLAG_DEFAULT | PAGE_FLAG_ZERO,
					  &tmpaddr);
		if (error != 0)
			return (error);
		pm->pm_level0[pml0i] = (struct pmap_lev0 *)tmpaddr;
	}
	pml0 = pmap_find0(pm, vaddr);
	pml1i = pmap_index1(vaddr);
	if (pml0->pml0_level1[pml1i] == NULL) {
		error = page_alloc_direct(&kernel_vm,
					  PAGE_FLAG_DEFAULT | PAGE_FLAG_ZERO,
					  &tmpaddr);
		if (error != 0) {
			/* XXX deallocate.  */
			return (error);
		}
		pml0->pml0_level1[pml1i] = (struct pmap_lev1 *)tmpaddr;
	}
	pml1 = pmap_find1(pml0, vaddr);
	if (ptep != NULL)
		*ptep = pmap_find_pte(pml1, vaddr);
	return (0);
}

static void
pmap_collect(struct pmap *pm)
{
	panic("%s: can't garbage collect yet.", __func__);
}

static struct pmap_lev0 *
pmap_find0(struct pmap *pm, vaddr_t vaddr)
{
	if (pm == NULL)
		return (NULL);
	return (pm->pm_level0[pmap_index0(vaddr)]);
}

static struct pmap_lev1 *
pmap_find1(struct pmap_lev0 *pml0, vaddr_t vaddr)
{
	if (pml0 == NULL)
		return (NULL);
	return (pml0->pml0_level1[pmap_index1(vaddr)]);
}

static pt_entry_t *
pmap_find_pte(struct pmap_lev1 *pml1, vaddr_t vaddr)
{
	if (pml1 == NULL)
		return (NULL);
	return (&pml1->pml1_entries[pmap_index_pte(vaddr)]);
}

static bool
pmap_is_direct(vaddr_t vaddr)
{
	if (vaddr >= XKPHYS_BASE && vaddr <= XKPHYS_END)
		return (true);
	return (false);
}

static void
pmap_pinit(struct pmap *pm, vaddr_t base, vaddr_t end)
{
	unsigned l0;

	if (pm != kernel_vm.vm_pmap) {
		pm->pm_active = 0;
	} else {
		pm->pm_active = mp_cpu_present_mask();
	}
	pmap_alloc_asid(pm); /* XXX Premature.  */
	ASSERT(pmap_index0(base) == 0, "Base must be aligned.");
	ASSERT(pmap_index1(base) == 0, "Base must be aligned.");
	ASSERT(pmap_index_pte(base) == 0, "Base must be aligned.");
	pm->pm_base = base;
	pm->pm_end = end;
	for (l0 = 0; l0 < NL0PMAP; l0++)
		pm->pm_level0[l0] = NULL;
}

static void
pmap_update(struct pmap *pm, vaddr_t vaddr, struct vm_page *page, pt_entry_t flags)
{
	pt_entry_t *pte;
	paddr_t opaddr, paddr;

	paddr = page_address(page);
	vaddr &= ~PAGE_MASK;

	pte = pmap_find(pm, vaddr);
	if (pte == NULL)
		panic("%s: update of PTE that isn't there.", __func__);

	opaddr = TLBLO_PTE_TO_PA(*pte);
	if (pte_test(pte, PG_V)) {
		if (opaddr == paddr) {
			/* Mapping stayed the same, just check flags.  */
			panic("%s: mapping stayed the same.", __func__);
			return;
		}
		tlb_invalidate(pm, vaddr);
	}
	atomic_store64(pte, TLBLO_PA_TO_PFN(paddr) | flags);
}

static void
pmap_startup(void *arg)
{
#ifdef VERBOSE
	printf("PMAP: initialization complete.\n");
#endif
	PCPU_SET(asidnext, PMAP_ASID_FIRST);
}
STARTUP_ITEM(pmap, STARTUP_PMAP, STARTUP_FIRST, pmap_startup, NULL);

#if defined(DB)
static void
db_pmap_dump_pte(vaddr_t base, pt_entry_t pte)
{
	if (pte == 0)
		return;
	printf("\t\t\t%p => %jx\n", (void *)base, (uintmax_t)pte);
}

static void
db_pmap_dump_level1(struct pmap_lev1 *pml1, vaddr_t base, unsigned level)
{
	unsigned i;

	if (level == 1) {
		printf("\t\t%p => %p\n", (void *)base, pml1);
		return;
	}

	printf("\t\tpage table entries:\n");
	for (i = 0; i < NPTEL1; i++) {
		db_pmap_dump_pte(base, pml1->pml1_entries[i]);
		base += PAGE_SIZE;
	}
}

static void
db_pmap_dump_level0(struct pmap_lev0 *pml0, vaddr_t base, unsigned level)
{
	unsigned i;

	if (level == 0) {
		printf("\t%p => %p\n", (void *)base, pml0);
		return;
	}

	printf("\tlevel 1 pointers:\n");
	for (i = 0; i < NL1PL0; i++) {
		struct pmap_lev1 *pml1 = pml0->pml0_level1[i];

		if (pml1 != NULL)
			db_pmap_dump_level1(pml1, base, level);
		base += PAGE_SIZE * NPTEL1;
	}
}

static void
db_pmap_dump_pmap(struct vm *vm, unsigned level)
{
	struct pmap *pm = vm->vm_pmap;
	vaddr_t base;
	unsigned i;

	printf("VM %p PMAP %p\n", vm, pm);
	if (pm == NULL)
		return;

	printf("\tasid %u begin %p end %p\n", pmap_asid(pm),
		 (void *)pm->pm_base, (void *)pm->pm_end);

	base = pm->pm_base;
	printf("level 0 pointers:\n");
	for (i = 0; i < NL0PMAP; i++) {
		struct pmap_lev0 *pml0 = pm->pm_level0[i];
		if (pml0 != NULL)
			db_pmap_dump_level0(pml0, base, level);
		base += PAGE_SIZE * NPTEL1 * NL1PL0;
	}
}

static void
db_pmap_dump_kvm_pte(void)
{
	db_pmap_dump_pmap(&kernel_vm, 2);
}
DB_COMMAND(kvm_pte, pmap, db_pmap_dump_kvm_pte);

static void
db_pmap_dump_kvm_l1(void)
{
	db_pmap_dump_pmap(&kernel_vm, 1);
}
DB_COMMAND(kvm_l1, pmap, db_pmap_dump_kvm_l1);

static void
db_pmap_dump_kvm_l0(void)
{
	db_pmap_dump_pmap(&kernel_vm, 0);
}
DB_COMMAND(kvm_l0, pmap, db_pmap_dump_kvm_l0);

static void
db_pmap_dump_task_pte(void)
{
	struct task *task;

	task = current_task();

	if (task == NULL) {
		printf("No current task.\n");
		return;
	}

	if (task->t_vm == NULL) {
		printf("Current task has no VM.\n");
		return;
	}
	db_pmap_dump_pmap(task->t_vm, 2);
}
DB_COMMAND(task_pte, pmap, db_pmap_dump_task_pte);

static void
db_pmap_dump_task_l1(void)
{
	struct task *task;

	task = current_task();

	if (task == NULL) {
		printf("No current task.\n");
		return;
	}

	if (task->t_vm == NULL) {
		printf("Current task has no VM.\n");
		return;
	}
	db_pmap_dump_pmap(task->t_vm, 1);
}
DB_COMMAND(task_l1, pmap, db_pmap_dump_task_l1);

static void
db_pmap_dump_task_l0(void)
{
	struct task *task;

	task = current_task();

	if (task == NULL) {
		printf("No current task.\n");
		return;
	}

	if (task->t_vm == NULL) {
		printf("Current task has no VM.\n");
		return;
	}
	db_pmap_dump_pmap(task->t_vm, 0);
}
DB_COMMAND(task_l0, pmap, db_pmap_dump_task_l0);
#endif

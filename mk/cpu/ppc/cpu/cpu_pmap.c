#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/pmap.h>
#ifdef VERBOSE
#include <core/console.h>
#endif
#include <vm/vm.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

/*
 * If it goes over PAGE_SIZE, we would need a way to allocate a contiguous
 * direct-mapped region, or we would have to have the page tables self-map the
 * pmap entry.
 */
COMPILE_TIME_ASSERT(sizeof (struct pmap) <= PAGE_SIZE);

static void pmap_pinit(struct pmap *, vaddr_t, vaddr_t);

static struct pool pmap_pool;

void
pmap_bootstrap(void)
{
	int error;

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
	return (ERROR_NOT_IMPLEMENTED);
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
	return (ERROR_NOT_IMPLEMENTED);
}

int
pmap_unmap(struct vm *vm, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

void
pmap_zero(struct vm_page *page)
{
	paddr_t paddr;
	uint64_t *p;
	size_t i;

	paddr = page_address(page);
	p = (uint64_t *)paddr;

	for (i = 0; i < (PAGE_SIZE / sizeof *p) / 16; i++) {
		p[0x0] = p[0x1] = p[0x2] = p[0x3] =
		p[0x4] = p[0x5] = p[0x6] = p[0x7] =
		p[0x8] = p[0x9] = p[0xa] = p[0xb] =
		p[0xc] = p[0xd] = p[0xe] = p[0xf] = 0x0;
		p += 16;
	}
}

static void
pmap_pinit(struct pmap *pm, vaddr_t base, vaddr_t end)
{
	pm->pm_base = base;
	pm->pm_end = end;
}

static void
pmap_startup(void *arg)
{
#ifdef VERBOSE
	kcprintf("PMAP: initialization complete.\n");
#endif
}
STARTUP_ITEM(pmap, STARTUP_PMAP, STARTUP_FIRST, pmap_startup, NULL);

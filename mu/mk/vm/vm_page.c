#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <db/db.h>
#include <db/db_show.h>
#include <io/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

DB_SHOW_TREE(vm_page, page);
DB_SHOW_VALUE_TREE(page, vm, DB_SHOW_TREE_POINTER(vm_page));

#define	VM_PAGE_ARRAY_ENTRIES	(PAGE_SIZE / sizeof (struct vm_page))
#define	VM_PAGE_ARRAY_COUNT	(1)

static struct vm_page_array {
	struct vm_page vmpa_pages[VM_PAGE_ARRAY_ENTRIES];
} page_array_pages[VM_PAGE_ARRAY_COUNT];
static unsigned page_array_count;

#if 0
COMPILE_TIME_ASSERT(sizeof (struct vm_page_array) == PAGE_SIZE);
#else
COMPILE_TIME_ASSERT(sizeof (struct vm_page_array) <= PAGE_SIZE);
#endif

static struct vm_pageq page_free_queue, page_use_queue;
static struct spinlock page_array_lock;
static struct spinlock page_queue_lock;

static struct vm_page *page_array_get_page(void);
static void page_insert(struct vm_page *, paddr_t);
static int page_lookup(paddr_t, struct vm_page **);
static int page_map_direct(struct vm *, struct vm_page *, vaddr_t *);
static void page_ref_drop(struct vm_page *);
static void page_ref_hold(struct vm_page *);
static int page_unmap_direct(struct vm *, struct vm_page *, vaddr_t);

#define	PAGE_ARRAY_LOCK()	spinlock_lock(&page_array_lock)
#define	PAGE_ARRAY_UNLOCK()	spinlock_unlock(&page_array_lock)

#define	PAGEQ_LOCK()	spinlock_lock(&page_queue_lock)
#define	PAGEQ_UNLOCK()	spinlock_unlock(&page_queue_lock)

void
page_init(void)
{
	spinlock_init(&page_array_lock, "Page array", SPINLOCK_FLAG_DEFAULT);
	spinlock_init(&page_queue_lock, "Page queue",
		      SPINLOCK_FLAG_DEFAULT | SPINLOCK_FLAG_RECURSE);

	TAILQ_INIT(&page_free_queue);
	TAILQ_INIT(&page_use_queue);

#ifdef	VERBOSE
	kcprintf("PAGE: page size is %uK, %lu pages per page array.\n",
		 PAGE_SIZE / 1024, VM_PAGE_ARRAY_ENTRIES);
#endif
}

paddr_t
page_address(struct vm_page *page)
{
	ASSERT(page != NULL, "Must have a page.");
	ASSERT(page->pg_refcnt != 0, "Cannot take address of unused page.");
	return (page->pg_addr);
}

int
page_alloc(struct vm *vm, unsigned flags, struct vm_page **pagep)
{
	struct vm_page *page;

	PAGEQ_LOCK();
	if (TAILQ_EMPTY(&page_free_queue)) {
		PAGEQ_UNLOCK();
		return (ERROR_EXHAUSTED);
	}

	page = TAILQ_FIRST(&page_free_queue);
	page_ref_hold(page);
	PAGEQ_UNLOCK();

	if ((flags & PAGE_FLAG_ZERO) != 0)
		pmap_zero(page);
	*pagep = page;
	return (0);
}

int
page_alloc_direct(struct vm *vm, unsigned flags, vaddr_t *vaddrp)
{
	struct vm_page *page;
	vaddr_t vaddr;
	int error, error2;

	error = page_alloc(vm, flags, &page);
	if (error != 0)
		return (error);

	error = page_map_direct(vm, page, &vaddr);
	if (error != 0) {
		error2 = page_release(vm, page);
		if (error2 != 0)
			panic("%s: can't release page: %m", __func__, error);
		return (error);
	}
	*vaddrp = vaddr;
	return (0);
}

int
page_extract(struct vm *vm, vaddr_t vaddr, struct vm_page **pagep)
{
	struct vm_page *page;
	paddr_t paddr;
	int error;

	error = pmap_extract(vm, vaddr, &paddr);
	if (error != 0)
		return (error);

	error = page_lookup(paddr, &page);
	if (error != 0)
		return (error);

	return (0);
}

int
page_free_direct(struct vm *vm, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");

	error = page_extract(vm, vaddr, &page);
	if (error != 0)
		return (error);

	error = page_release(vm, page);
	if (error != 0)
		return (error);

	return (0);
}

int
page_insert_pages(paddr_t base, size_t pages)
{
	struct vm_page_array *vpa;
	struct vm_page *page;
	size_t inserted;
	vaddr_t va;
	unsigned i;
	int error;

	inserted = 0;

	ASSERT(PAGE_ALIGNED(base), "must be a page address");

	for (;;) {
		if (pages == 1) {
			kcprintf("PAGE: no array for %p.\n", (void *)base);
			break;
		}
		PAGE_ARRAY_LOCK();
		page = page_array_get_page();
		page_insert(page, base);
		page_ref_hold(page);
		PAGE_ARRAY_UNLOCK();
		error = page_map_direct(&kernel_vm, page, &va);
		if (error != 0)
			panic("%s: failed to map page array: %m", __func__,
			      error);
		vpa = (struct vm_page_array *)va;
		base += PAGE_SIZE;
		pages--;
		for (i = 0; i < VM_PAGE_ARRAY_ENTRIES; i++) {
			if (pages == 0) {
				page_ref_drop(page);
				break;
			}
			PAGE_ARRAY_LOCK();
			page_ref_hold(page);
			page_insert(&vpa->vmpa_pages[i], base);
			PAGE_ARRAY_UNLOCK();
			base += PAGE_SIZE;
			pages--;
			inserted++;
		}
		page_ref_drop(page);
		if (pages == 0)
			break;
	}

#ifdef	VERBOSE
	kcprintf("PAGE: inserted %lu pages (%luM)\n", inserted,
		 (inserted * PAGE_SIZE) / (1024 * 1024));
	kcprintf("PAGE: using %u arrays (%u, %u)\n", page_array_count,
		 page_array_count / VM_PAGE_ARRAY_COUNT,
		 page_array_count % VM_PAGE_ARRAY_COUNT);
#endif
	return (0);
}

int
page_map(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");
	page_ref_hold(page);
	error = pmap_map(vm, vaddr, page);
	if (error == 0)
		return (0);
	page_ref_drop(page);
	return (error);
}

int
page_release(struct vm *vm, struct vm_page *page)
{
	page_ref_drop(page);
	ASSERT(page->pg_refcnt == 0, "Cannot release if refs held.");
	return (0);
}

int
page_unmap(struct vm *vm, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");

	error = page_extract(vm, vaddr, &page);
	if (error != 0)
		return (error);

	error = pmap_unmap(vm, vaddr);
	if (error != 0)
		return (error);

	page_ref_drop(page);
	return (0);
}

static struct vm_page *
page_array_get_page(void)
{
	struct vm_page_array *vpa;
	struct vm_page *page;
	unsigned i;

	ASSERT(page_array_count !=
	       (VM_PAGE_ARRAY_ENTRIES * VM_PAGE_ARRAY_COUNT),
	       "Too many page arrays.");

	i = page_array_count++;
	vpa = &page_array_pages[i / VM_PAGE_ARRAY_ENTRIES];
	page = &vpa->vmpa_pages[i % VM_PAGE_ARRAY_ENTRIES];
	return (page);
}

static void
page_insert(struct vm_page *page, paddr_t paddr)
{
	page->pg_addr = paddr;
	page->pg_refcnt = 0;
	TAILQ_INSERT_TAIL(&page_free_queue, page, pg_link);
}

static int
page_lookup(paddr_t paddr, struct vm_page **pagep)
{
	struct vm_page_array *vmpa, *vmpa2;
	struct vm_page *page, *page2;
	unsigned i, j, k;
	vaddr_t va;
	int error;

	ASSERT(PAGE_ALIGNED(paddr), "must be a page address");

	PAGE_ARRAY_LOCK();
	for (i = 0; i < VM_PAGE_ARRAY_COUNT; i++) {
		vmpa = &page_array_pages[i];
		for (j = 0; j < VM_PAGE_ARRAY_ENTRIES; j++) {
			page = &vmpa->vmpa_pages[j];
			if (page->pg_refcnt == 0)
				continue;
			if (page_address(page) == paddr) {
				*pagep = page;
				PAGE_ARRAY_UNLOCK();
				return (0);
			}

			error = page_map_direct(&kernel_vm, page, &va);
			if (error != 0)
				panic("%s: page_map_direct failed: %m",
				      __func__, error);
			vmpa2 = (struct vm_page_array *)va;
			for (k = 0; k < VM_PAGE_ARRAY_ENTRIES; k++) {
				page2 = &vmpa2->vmpa_pages[k];
				if (page2->pg_refcnt == 0)
					continue;
				if (page_address(page2) == paddr) {
					*pagep = page2;
					PAGE_ARRAY_UNLOCK();
					return (0);
				}
			}
			error = page_unmap_direct(&kernel_vm, page, va);
			if (error != 0)
				panic("%s: page_unmap_direct failed: %m",
				      __func__, error);
		}
	}
	PAGE_ARRAY_UNLOCK();
	return (ERROR_NOT_FOUND);
}

static int
page_map_direct(struct vm *vm, struct vm_page *page, vaddr_t *vaddrp)
{
	int error;

	if (vm != &kernel_vm)
		panic("%s: can't direct map for non-kernel address space.",
		      __func__);
	page_ref_hold(page);
	error = pmap_map_direct(vm, page, vaddrp);
	if (error != 0)
		page_ref_drop(page);
	return (error);
}

static void
page_ref_drop(struct vm_page *page)
{
	PAGEQ_LOCK();
	ASSERT(page->pg_refcnt != 0, "Cannot drop refcount on unheld page.");
	page->pg_refcnt--;
	if (page->pg_refcnt == 0) {
		TAILQ_REMOVE(&page_use_queue, page, pg_link);
		TAILQ_INSERT_TAIL(&page_free_queue, page, pg_link);
	}
	PAGEQ_UNLOCK();
}

static void
page_ref_hold(struct vm_page *page)
{
	PAGEQ_LOCK();
	if (page->pg_refcnt == 0) {
		TAILQ_REMOVE(&page_free_queue, page, pg_link);
		TAILQ_INSERT_TAIL(&page_use_queue, page, pg_link);
	}
	page->pg_refcnt++;
	ASSERT(page->pg_refcnt != 0, "Refcount wrapped.");
	PAGEQ_UNLOCK();
}

static int
page_unmap_direct(struct vm *vm, struct vm_page *page, vaddr_t vaddr)
{
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");
	error = pmap_unmap_direct(vm, vaddr);
	if (error == 0)
		page_ref_drop(page);
	return (error);
}

static void
db_vm_page_dump(struct vm_page *page)
{
	kcprintf("vm_page %p addr %p refcnt %u\n", page, (void *)page->pg_addr,
		 page->pg_refcnt);
}

static void
db_vm_page_dump_queue(struct vm_pageq *pq)
{
	struct vm_page *page;

	TAILQ_FOREACH(page, pq, pg_link)
		db_vm_page_dump(page);
}

static void
db_vm_page_dump_arrays(void)
{
	unsigned i, j;

	kcprintf("page array count = %u/%u\n", page_array_count,
		 VM_PAGE_ARRAY_COUNT);
	for (i = 0; i < VM_PAGE_ARRAY_COUNT; i++) {
		kcprintf("page array #%u\n", i);
		for (j = 0; j < VM_PAGE_ARRAY_ENTRIES; j++) {
			kcprintf("page #%u\n", j);
			db_vm_page_dump(&page_array_pages[i].vmpa_pages[j]);
		}
	}
}
DB_SHOW_VALUE_VOIDF(arrays, vm_page, db_vm_page_dump_arrays);

static void
db_vm_page_dump_freeq(void)
{
	db_vm_page_dump_queue(&page_free_queue);
}
DB_SHOW_VALUE_VOIDF(freeq, vm_page, db_vm_page_dump_freeq);

static void
db_vm_page_dump_useq(void)
{
	db_vm_page_dump_queue(&page_use_queue);
}
DB_SHOW_VALUE_VOIDF(useq, vm_page, db_vm_page_dump_useq);

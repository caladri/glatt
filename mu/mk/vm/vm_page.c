#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	VM_PAGE_ARRAY_ENTRIES	(PAGE_SIZE / sizeof (struct vm_page))
#define	VM_PAGE_ARRAY_COUNT	(1024)

static struct vm_page_array {
	struct vm_page vmpa_pages[VM_PAGE_ARRAY_ENTRIES];
} page_array_pages[VM_PAGE_ARRAY_COUNT];
static unsigned page_array_count;

COMPILE_TIME_ASSERT(sizeof (struct vm_page_array) == PAGE_SIZE);

static TAILQ_HEAD(, struct vm_page) page_free_queue;
static TAILQ_HEAD(, struct vm_page) page_use_queue;
static struct spinlock page_lock = SPINLOCK_INIT("PAGE");

static struct vm_page *page_array_get_page(void);
static void page_insert(struct vm_page *, paddr_t, uint32_t);
static void page_ref_drop(struct vm_page *);
static void page_ref_hold(struct vm_page *);

#define	PAGE_LOCK()	spinlock_lock(&page_lock)
#define	PAGE_UNLOCK()	spinlock_unlock(&page_lock)

void
page_init(void)
{
	TAILQ_INIT(&page_free_queue);
	TAILQ_INIT(&page_use_queue);
	kcprintf("PAGE: page size is %uK, %lu pages per page array.\n",
		 PAGE_SIZE / 1024, VM_PAGE_ARRAY_ENTRIES);
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

	PAGE_LOCK();
	if (TAILQ_EMPTY(&page_free_queue)) {
		PAGE_UNLOCK();
		return (ERROR_EXHAUSTED);
	}

	page = TAILQ_FIRST(&page_free_queue);
	page_ref_hold(page);
	PAGE_UNLOCK();
	if ((flags & PAGE_FLAG_ZERO) != 0)
		page_zero(page);
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
	panic("%s: unimplemented.", __func__);
#if 0
	int error;

	PAGE_LOCK();
	error = pmap_extract(vm, PAGE_FLOOR(vaddr), pagep);
	PAGE_UNLOCK();
	*paddrp |= PAGE_OFFSET(vaddr);
	return (error);
#else
	return (ERROR_NOT_IMPLEMENTED);
#endif
}

int
page_free_direct(struct vm *vm, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	ASSERT(PAGE_OFFSET(vaddr) == 0, "must be a page address");

	error = page_extract(vm, vaddr, &page);
	if (error != 0)
		return (error);

	error = page_unmap_direct(vm, vaddr);
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

	ASSERT(PAGE_OFFSET(base) == 0, "must be a page address");
	for (;;) {
		if (pages == 1) {
			kcprintf("PAGE: no array for %p.\n", (void *)base);
			break;
		}
		PAGE_LOCK();
		page = page_array_get_page();
		page_insert(page, base, VM_PAGE_DEFAULT);
		page_ref_hold(page);
		PAGE_UNLOCK();
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
			PAGE_LOCK();
			page_ref_hold(page);
			page_insert(&vpa->vmpa_pages[i], base, VM_PAGE_DEFAULT);
			PAGE_UNLOCK();
			base += PAGE_SIZE;
			pages--;
			inserted++;
		}
		page_ref_drop(page);
		if (pages == 0)
			break;
	}

	kcprintf("PAGE: inserted %lu pages (%luM)\n", inserted,
		 (inserted * PAGE_SIZE) / (1024 * 1024));
	kcprintf("PAGE: using %u arrays (%u, %u)\n", page_array_count,
		 page_array_count / VM_PAGE_ARRAY_COUNT,
		 page_array_count % VM_PAGE_ARRAY_COUNT);
	return (0);
}

int
page_map(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	int error;

	ASSERT(PAGE_OFFSET(vaddr) == 0, "must be a page address");
	PAGE_LOCK();
	page_ref_hold(page);
	error = pmap_map(vm, vaddr, page);
	if (error != 0)
		page_ref_drop(page);
	PAGE_UNLOCK();
	return (error);
}

int
page_map_direct(struct vm *vm, struct vm_page *page, vaddr_t *vaddrp)
{
	int error;

	if (vm != &kernel_vm)
		panic("%s: can't direct map for non-kernel address space.",
		      __func__);
	PAGE_LOCK();
	page_ref_hold(page);
	error = pmap_map_direct(vm, page, vaddrp);
	if (error != 0)
		page_ref_drop(page);
	PAGE_UNLOCK();
	return (error);
}

int
page_release(struct vm *vm, struct vm_page *page)
{
	PAGE_LOCK();
	page_ref_drop(page);
	ASSERT(page->pg_refcnt == 0, "Cannot release if refs held.");
	PAGE_UNLOCK();
	return (0);
}

int
page_unmap(struct vm *vm, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	ASSERT(PAGE_OFFSET(vaddr) == 0, "must be a page address");
	PAGE_LOCK();
	error = page_extract(vm, vaddr, &page);
	if (error == 0) {
		error = pmap_unmap(vm, vaddr);
		page_ref_drop(page);
	}
	PAGE_UNLOCK();
	return (error);
}

int
page_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	ASSERT(PAGE_OFFSET(vaddr) == 0, "must be a page address");
	PAGE_LOCK();
	error = page_extract(vm, vaddr, &page);
	if (error == 0) {
		error = pmap_unmap_direct(vm, vaddr);
		page_ref_drop(page);
	}
	PAGE_UNLOCK();
	return (error);
}

void
page_zero(struct vm_page *page)
{
	/*
	 * XXX check VM_PAGE_ZERO
	 */
	page_ref_hold(page);
	pmap_zero(page);
	page_ref_drop(page);
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
	vpa = &page_array_pages[i / VM_PAGE_ARRAY_COUNT];
	page = &vpa->vmpa_pages[i % VM_PAGE_ARRAY_COUNT];
	return (page);
}

static void
page_insert(struct vm_page *page, paddr_t paddr, uint32_t flags)
{
	page->pg_addr = paddr;
	page->pg_flags = flags;
	page->pg_refcnt = 0;
	TAILQ_INSERT_TAIL(&page_free_queue, page, pg_link);
}

static void
page_ref_drop(struct vm_page *page)
{
	PAGE_LOCK();
	ASSERT(page->pg_refcnt != 0, "Cannot drop refcount on unheld page.");
	page->pg_refcnt--;
	if (page->pg_refcnt == 0) {
		TAILQ_REMOVE(&page_use_queue, page, pg_link);
		TAILQ_INSERT_TAIL(&page_free_queue, page, pg_link);
	}
	PAGE_UNLOCK();
}

static void
page_ref_hold(struct vm_page *page)
{
	PAGE_LOCK();
	if (page->pg_refcnt == 0) {
		TAILQ_REMOVE(&page_free_queue, page, pg_link);
		TAILQ_INSERT_TAIL(&page_use_queue, page, pg_link);
	}
	page->pg_refcnt++;
	ASSERT(page->pg_refcnt != 0, "Refcount wrapped.");
	PAGE_UNLOCK();
}

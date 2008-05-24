#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/string.h>
#include <db/db.h>
#include <db/db_show.h>
#include <io/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

struct vm_page_tree_page;

DB_SHOW_TREE(vm_page, page);
DB_SHOW_VALUE_TREE(page, vm, DB_SHOW_TREE_POINTER(vm_page));

struct vm_page_tree {
	BTREE(struct vm_page_tree_page) pt_tree;
};

#define	VM_PAGE_TREE_ENTRIES	((PAGE_SIZE - sizeof (struct vm_page_tree)) / \
				  sizeof (struct vm_page))
struct vm_page_tree_page {
	struct vm_page_tree ptp_tree;
	struct vm_page ptp_entries[VM_PAGE_TREE_ENTRIES];
};

COMPILE_TIME_ASSERT(sizeof (struct vm_page_tree_page) <= PAGE_SIZE);

static struct vm_page_tree_page *page_tree;
static struct vm_pageq page_free_queue, page_use_queue;
static struct spinlock page_tree_lock;
static struct spinlock page_queue_lock;

static void page_insert(struct vm_page *, paddr_t);
static int page_lookup(paddr_t, struct vm_page **);
static int page_map_direct(struct vm *, struct vm_page *, vaddr_t *);
static void page_ref_drop(struct vm_page *);
static void page_ref_hold(struct vm_page *);
static int page_unmap_direct(struct vm *, struct vm_page *, vaddr_t);

#define	PAGE_TREE_LOCK()	spinlock_lock(&page_tree_lock)
#define	PAGE_TREE_UNLOCK()	spinlock_unlock(&page_tree_lock)

#define	PAGEQ_LOCK()	spinlock_lock(&page_queue_lock)
#define	PAGEQ_UNLOCK()	spinlock_unlock(&page_queue_lock)

void
page_init(void)
{
	spinlock_init(&page_tree_lock, "Page tree", SPINLOCK_FLAG_DEFAULT);
	spinlock_init(&page_queue_lock, "Page queue",
		      SPINLOCK_FLAG_DEFAULT | SPINLOCK_FLAG_RECURSE);

	TAILQ_INIT(&page_free_queue);
	TAILQ_INIT(&page_use_queue);

#ifdef	VERBOSE
	kcprintf("PAGE: page size is %uK, %lu pages per page tree.\n",
		 PAGE_SIZE / 1024, VM_PAGE_TREE_ENTRIES);
#endif
}

paddr_t
page_address(struct vm_page *page)
{
	struct vm_page_tree_page *ptp;
	paddr_t paddr;
	vaddr_t vaddr;
	int error;

	ASSERT(page != NULL, "Must have a page.");

	/*
	 * vm_pages are allocated linearly inside a vm_page_tree_page, and the
	 * 0th entry in the ptp_entries array is that vm_page_tree_page.  If
	 * you subtract them, you get which element it is (say, nth) within the
	 * ptp_entires array, which means your page is m+n where m is the page
	 * number of the vm_page_tree_page.
	 */
	vaddr = PAGE_FLOOR((vaddr_t)page);
	error = pmap_extract_direct(&kernel_vm, vaddr, &paddr);
	if (error != 0)
		panic("%s: pmap_extract failed: %m", __func__, error);

	ptp = (struct vm_page_tree_page *)vaddr;
	paddr += PAGE_SIZE * (page - &ptp->ptp_entries[0]);
	return (paddr);
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

	error = page_unmap_direct(vm, page, vaddr);
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
	struct vm_page_tree_page *ptp, *iter;
	struct vm_page_tree *pt;
	size_t ptps, ptpents;
	vaddr_t vaddr;
	unsigned i;
	int error;

	ASSERT(PAGE_ALIGNED(base), "must be a page address");

	ptps = ptpents = 0;

	PAGE_TREE_LOCK();
	while (pages > 1) {
		error = pmap_map_direct(&kernel_vm, base, &vaddr);
		if (error != 0)
			panic("%s: pmap_map_direct failed: %m", __func__,
			      error);

		ptp = (struct vm_page_tree_page *)vaddr;
		pt = &ptp->ptp_tree;
		BTREE_INIT(&ptp->ptp_tree.pt_tree);

		ptps++;

		for (i = 0; i < VM_PAGE_TREE_ENTRIES; i++) {
			struct vm_page *page;

			page = &ptp->ptp_entries[i];
			page_insert(page, base);
			if (i == 0) {
				/*
				 * The 0th page here is the one that these
				 * entires are in.
				 */
				page_ref_hold(page);
			}

			ptpents++;
			pages--;
			base += PAGE_SIZE;
			if (pages == 0)
				break;
		}
		BTREE_INSERT(ptp, iter, &page_tree, ptp_tree.pt_tree,
			     (ptp < iter));
	}
	PAGE_TREE_UNLOCK();

#if VERBOSE
	kcprintf("PAGE: inserted %zu page tree entries in %zu nodes.\n",
		 ptpents, ptps);
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

static void
page_insert(struct vm_page *page, paddr_t paddr)
{
	page->pg_refcnt = 0;
	TAILQ_INSERT_TAIL(&page_free_queue, page, pg_link);
}

static int
page_lookup(paddr_t paddr, struct vm_page **pagep)
{
	ASSERT(PAGE_ALIGNED(paddr), "must be a page address");

	panic("%s: not yet implemented.", __func__);
}

static int
page_map_direct(struct vm *vm, struct vm_page *page, vaddr_t *vaddrp)
{
	int error;

	if (vm != &kernel_vm)
		panic("%s: can't direct map for non-kernel address space.",
		      __func__);
	page_ref_hold(page);
	error = pmap_map_direct(vm, page_address(page), vaddrp);
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
	kcprintf("vm_page %p addr %p refcnt %u\n", page, page_address(page),
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

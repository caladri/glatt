#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/string.h>
#include <cpu/pmap.h>
#ifdef DB
#include <db/db_command.h>
#endif
#if defined(DB) || defined(VERBOSE)
#include <core/console.h>
#endif
#include <vm/vm.h>
#include <vm/vm_page.h>

struct vm_page_tree_page;

#ifdef DB
DB_COMMAND_TREE(page, vm, vm_page);
#endif

struct vm_page_queue {
	TAILQ_HEAD(, struct vm_page) pq_queue;
};

struct vm_page_tree {
	BTREE_NODE(struct vm_page_tree_page) pt_tree;
};

#define	VM_PAGE_TREE_ENTRIES	((PAGE_SIZE - sizeof (struct vm_page_tree)) / \
				  sizeof (struct vm_page))
struct vm_page_tree_page {
	struct vm_page_tree ptp_tree;
	struct vm_page ptp_entries[VM_PAGE_TREE_ENTRIES];
};

COMPILE_TIME_ASSERT(sizeof (struct vm_page_tree_page) <= PAGE_SIZE);

static BTREE_ROOT(struct vm_page_tree_page) page_tree;
static struct vm_page_queue page_free_queue, page_use_queue;
static struct spinlock page_queue_lock;

static void page_insert(struct vm_page *, paddr_t);
static int page_lookup(paddr_t, struct vm_page **);
static char page_lookup_cmp(paddr_t, struct vm_page_tree_page *,
			    struct vm_page **);
static void page_ref_drop(struct vm_page *);
static void page_ref_hold(struct vm_page *);

#define	PAGEQ_LOCK()	spinlock_lock(&page_queue_lock)
#define	PAGEQ_UNLOCK()	spinlock_unlock(&page_queue_lock)

void
page_init(void)
{
	spinlock_init(&page_queue_lock, "Page queue", SPINLOCK_FLAG_DEFAULT);

	BTREE_ROOT_INIT(&page_tree);

	TAILQ_INIT(&page_free_queue.pq_queue);
	TAILQ_INIT(&page_use_queue.pq_queue);

#ifdef VERBOSE_DEBUG
	printf("PAGE: page size is %uK, %lu pages per page tree.\n",
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
page_alloc(unsigned flags, struct vm_page **pagep)
{
	struct vm_page *page;

	PAGEQ_LOCK();
	if (TAILQ_EMPTY(&page_free_queue.pq_queue)) {
		PAGEQ_UNLOCK();
		return (ERROR_EXHAUSTED);
	}

	page = TAILQ_FIRST(&page_free_queue.pq_queue);
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
	int error;

	error = page_alloc(flags, &page);
	if (error != 0)
		return (error);

	error = page_map_direct(vm, page, &vaddr);
	if (error != 0) {
		page_release(page);
		return (error);
	}
	*vaddrp = vaddr;
	return (0);
}

int
page_alloc_map(struct vm *vm, unsigned flags, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	error = page_alloc(flags, &page);
	if (error != 0)
		return (error);

	error = page_map(vm, vaddr, page);
	if (error != 0) {
		page_release(page);
		return (error);
	}
	return (0);
}

int
page_clone(struct vm *vm, vaddr_t vaddr, struct vm_page **pagep)
{
	struct vm_page *page;
	vaddr_t src, dst;
	paddr_t psrc, pdst;
	int error, error2;

	if (vm != &kernel_vm) {
		/*
		 * XXX Locking?
		 */
		error = pmap_extract(vm, vaddr, &psrc);
		if (error != 0)
			return (error);

		error = pmap_map_direct(&kernel_vm, psrc, &src);
		if (error != 0)
			return (error);
	} else {
		src = vaddr;
	}

	error = page_alloc(PAGE_FLAG_DEFAULT, &page);
	if (error != 0) {
		if (vm != &kernel_vm) {
			error2 = pmap_unmap_direct(&kernel_vm, src);
			if (error2 != 0)
				panic("%s: pmap_unmap_direct failed: %m", __func__, error);
		}
		return (error);
	}

	pdst = page_address(page);
	error = pmap_map_direct(&kernel_vm, pdst, &dst);
	if (error != 0) {
		if (vm != &kernel_vm) {
			error2 = pmap_unmap_direct(&kernel_vm, src);
			if (error2 != 0)
				panic("%s: pmap_unmap_direct failed: %m", __func__, error);
		}
		page_release(page);
		return (error);
	}

	memcpy((void *)dst, (void *)src, PAGE_SIZE);

	if (vm != &kernel_vm) {
		error = pmap_unmap_direct(&kernel_vm, src);
		if (error != 0)
			panic("%s: pmap_unmap_direct failed: %m", __func__, error);
	}

	error = pmap_unmap_direct(&kernel_vm, dst);
	if (error != 0)
		panic("%s: pmap_unmap_direct failed: %m", __func__, error);

	*pagep = page;

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

	*pagep = page;

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

	page_release(page);

	return (0);
}

int
page_free_map(struct vm *vm, vaddr_t vaddr)
{
	struct vm_page *page;
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");

	error = page_extract(vm, vaddr, &page);
	if (error != 0)
		return (error);

	error = page_unmap(vm, vaddr, page);
	if (error != 0)
		return (error);

	page_release(page);

	return (0);
}

int
page_insert_pages(paddr_t base, size_t pages)
{
	struct vm_page_tree_page *ptp, *iter;
	size_t ptps, ptpents;
	vaddr_t vaddr;
	unsigned i;
	int error;

	ASSERT(PAGE_ALIGNED(base), "must be a page address");

	ptps = ptpents = 0;

	PAGEQ_LOCK();
	while (pages > 1) {
		error = pmap_map_direct(&kernel_vm, base, &vaddr);
		if (error != 0)
			panic("%s: pmap_map_direct failed: %m", __func__,
			      error);

		ptp = (struct vm_page_tree_page *)vaddr;
		BTREE_NODE_INIT(&ptp->ptp_tree.pt_tree);

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
	PAGEQ_UNLOCK();

#ifdef VERBOSE
	printf("PAGE: inserted %zu page tree entries in %zu nodes.\n",
		 ptpents, ptps);
#endif

	return (0);
}

int
page_map(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");
	PAGEQ_LOCK();
	page_ref_hold(page);
	PAGEQ_UNLOCK();
	error = pmap_map(vm, vaddr, page);
	if (error != 0) {
		PAGEQ_LOCK();
		page_ref_drop(page);
		PAGEQ_UNLOCK();
		return (error);
	}
	return (0);
}

int
page_map_direct(struct vm *vm, struct vm_page *page, vaddr_t *vaddrp)
{
	int error;

	if (vm != &kernel_vm)
		panic("%s: can't direct map for non-kernel address space.",
		      __func__);
	PAGEQ_LOCK();
	page_ref_hold(page);
	PAGEQ_UNLOCK();
	error = pmap_map_direct(vm, page_address(page), vaddrp);
	if (error != 0) {
		PAGEQ_LOCK();
		page_ref_drop(page);
		PAGEQ_UNLOCK();
	}
	return (error);
}

void
page_release(struct vm_page *page)
{
	PAGEQ_LOCK();
	page_ref_drop(page);
	ASSERT(page->pg_refcnt == 0, "Cannot release if refs held.");
	PAGEQ_UNLOCK();
}

int
page_unmap(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");

	error = pmap_unmap(vm, vaddr);
	if (error != 0)
		return (error);

	PAGEQ_LOCK();
	page_ref_drop(page);
	PAGEQ_UNLOCK();
	return (0);
}

int
page_unmap_direct(struct vm *vm, struct vm_page *page, vaddr_t vaddr)
{
	int error;

	ASSERT(PAGE_ALIGNED(vaddr), "must be a page address");
	error = pmap_unmap_direct(vm, vaddr);
	if (error != 0)
		panic("%s: pmap_unmap_direct failed: %m", __func__, error);
	PAGEQ_LOCK();
	page_ref_drop(page);
	PAGEQ_UNLOCK();
	return (0);
}

static void
page_insert(struct vm_page *page, paddr_t paddr)
{
	SPINLOCK_ASSERT_HELD(&page_queue_lock);
	page->pg_refcnt = 0;
	TAILQ_INSERT_TAIL(&page_free_queue.pq_queue, page, pg_link);
}

static int
page_lookup(paddr_t paddr, struct vm_page **pagep)
{
	struct vm_page_tree_page *ptp, *iter;

	ASSERT(PAGE_ALIGNED(paddr), "must be a page address");

	BTREE_FIND(&ptp, iter, &page_tree, ptp_tree.pt_tree,
		   page_lookup_cmp(paddr, iter, pagep) == '<',
		   page_lookup_cmp(paddr, iter, pagep) == '=');

	if (ptp == NULL)
		return (ERROR_NOT_FOUND);
	return (0);
}

static char
page_lookup_cmp(paddr_t paddr, struct vm_page_tree_page *ptp,
		struct vm_page **pagep)
{
	unsigned entry;
	paddr_t base;

	base = page_address(&ptp->ptp_entries[0]);

	if (paddr < base)
		return ('<');

	entry = ADDR_TO_PAGE(paddr - base);

	/*
	 * XXX
	 * This does not handle the case where paddr could be validly
	 * represented in two (or more) different page tree pages due to
	 * page_insert_pages being called for nearby areas multiple times.
	 *
	 * The correct way to handle it, of course, is to never let that sort
	 * of thing happen and to fix page_insert_pages to always insert into
	 * an existing page tree page that covers an address space if there is
	 * one.  Luckily, this can be done with our new page_lookup.  Right?
	 */
	if (entry < VM_PAGE_TREE_ENTRIES) {
		*pagep = &ptp->ptp_entries[entry];
		return ('=');
	}

	return ('>');
}

static void
page_ref_drop(struct vm_page *page)
{
	SPINLOCK_ASSERT_HELD(&page_queue_lock);

	ASSERT(page->pg_refcnt != 0, "Cannot drop refcount on unheld page.");
	page->pg_refcnt--;
	if (page->pg_refcnt == 0) {
		TAILQ_REMOVE(&page_use_queue.pq_queue, page, pg_link);
		TAILQ_INSERT_TAIL(&page_free_queue.pq_queue, page, pg_link);
	}
}

static void
page_ref_hold(struct vm_page *page)
{
	SPINLOCK_ASSERT_HELD(&page_queue_lock);

	if (page->pg_refcnt == 0) {
		TAILQ_REMOVE(&page_free_queue.pq_queue, page, pg_link);
		TAILQ_INSERT_TAIL(&page_use_queue.pq_queue, page, pg_link);
	}
	page->pg_refcnt++;
	ASSERT(page->pg_refcnt != 0, "Refcount wrapped.");
}

#ifdef DB
static void
db_vm_page_dump(struct vm_page *page)
{
	printf("vm_page %p addr %p refcnt %u\n", page, page_address(page),
		 page->pg_refcnt);
}

static void
db_vm_page_dump_queue(struct vm_page_queue *pq)
{
	struct vm_page *page;

	TAILQ_FOREACH(page, &pq->pq_queue, pg_link)
		db_vm_page_dump(page);
}

static void
db_vm_page_dump_freeq(void)
{
	db_vm_page_dump_queue(&page_free_queue);
}
DB_COMMAND(freeq, vm_page, db_vm_page_dump_freeq);

static void
db_vm_page_dump_useq(void)
{
	db_vm_page_dump_queue(&page_use_queue);
}
DB_COMMAND(useq, vm_page, db_vm_page_dump_useq);
#endif

#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

#include <io/device/console/console.h>

#define	POOL_LOCK(pool)		spinlock_lock(&(pool)->pool_lock)
#define	POOL_UNLOCK(pool)	spinlock_unlock(&(pool)->pool_lock)

#define	POOL_ITEM_DEFAULT	(0x0000)
#define	POOL_ITEM_FREE		(0x0001)

struct pool_item {
	uint16_t pi_flags;
	uint16_t pi_unused16;
	uint32_t pi_unused32;
};
COMPILE_TIME_ASSERT(sizeof (struct pool_item) == 8);

#define	POOL_PAGE_MAGIC		(0x19800604)
struct pool_page {
	uint32_t pp_magic;
	struct vm_page *pp_backing;
	struct pool *pp_pool;
	SLIST_ENTRY(struct pool_page) pp_link;
	size_t pp_items;
	/* XXX push into 'struct pool' once we nuke static initializers.  */
	size_t pp_maxitems;
};

#define	MAX_ALLOC_SIZE	((PAGE_SIZE - (sizeof (struct pool_page) +	\
				       (sizeof (struct pool_item) * 2)))\
			 / 2)

static struct pool_item *pool_get(struct pool *);
static void pool_initialize_page(struct pool_page *);
static struct pool_page *pool_page(struct pool_item *);
static struct pool_item *pool_page_item(struct pool_page *, unsigned);

size_t pool_max_alloc = MAX_ALLOC_SIZE;

void *
pool_allocate(struct pool *pool)
{
	struct pool_item *item;
	struct pool_page *page;
	struct vm_page *backing;
	vaddr_t vaddr;
	int error;

	if ((pool->pool_flags & POOL_VALID) == 0)
		panic("%s: pool %s used before initialization!", __func__,
		      pool->pool_name);
	ASSERT(pool->pool_size <= MAX_ALLOC_SIZE, "pool must not be so big.");

	POOL_LOCK(pool);
	item = pool_get(pool);
	if (item++ != NULL) {
		POOL_UNLOCK(pool);
		return ((void *)item);
	}
	error = page_alloc(&kernel_vm, PAGE_FLAG_DEFAULT, &backing);
	if (error != 0) {
		/* XXX check whether we could block, try to GC some shit.  */
		POOL_UNLOCK(pool);
		return (NULL);
	}
	if ((pool->pool_flags & POOL_VIRTUAL) != 0) {
		error = vm_page_map(&kernel_vm, backing, &vaddr);
	} else {
		error = page_map_direct(&kernel_vm, backing, &vaddr);
	}
	if (error != 0)
		panic("%s: can't map page for allocation: %m", __func__, error);

	page = (struct pool_page *)vaddr;
	page->pp_magic = POOL_PAGE_MAGIC;
	page->pp_backing = backing;
	page->pp_pool = pool;
	SLIST_INSERT_HEAD(&pool->pool_pages, page, pp_link);
	page->pp_items = 0;
	page->pp_maxitems = (PAGE_SIZE - sizeof (struct pool_page)) /
		(pool->pool_size + sizeof (struct pool_item));
	pool_initialize_page(page);

	item = pool_get(pool);
	if (item++ == NULL)
		panic("%s: can't get memory but we just allocated!", __func__);
	POOL_UNLOCK(pool);
	return ((void *)item);
}

void
pool_free(void *m)
{
	struct pool_item *item;
	struct pool_page *page;
	struct pool *pool;
	struct vm_page *backing;
	vaddr_t vaddr;
	int error;

	item = m;
	item--;
	page = pool_page(item);
	pool = page->pp_pool;
	ASSERT((pool->pool_flags & POOL_VALID) != 0, "pool must be valid.");
	POOL_LOCK(pool);
	item->pi_flags |= POOL_ITEM_FREE;
	if (page->pp_items == 0)
		panic("%s: pool %s has no items.", __func__, pool->pool_name);
	if (page->pp_items-- != 1) {
		POOL_UNLOCK(pool);
		return;
	}
	/*
	 * Last item in page, unmap it, free any virtual addresses and put the
	 * page itself back into the free pool.
	 */
	page->pp_magic = ~POOL_PAGE_MAGIC;
	SLIST_REMOVE(&pool->pool_pages, page, struct pool_page, pp_link);
	backing = page->pp_backing;
	vaddr = (vaddr_t)page;
	if ((pool->pool_flags & POOL_VIRTUAL) != 0) {
		error = vm_page_unmap(&kernel_vm, vaddr);
	} else {
		error = page_unmap_direct(&kernel_vm, vaddr);
	}
	if (error != 0)
		panic("%s: can't unmap page: %m", __func__, error);
	error = page_release(&kernel_vm, backing);
	if (error != 0)
		panic("%s: page_release failed: %m", __func__, error);
	POOL_UNLOCK(pool);
}

int
pool_create(struct pool *pool, const char *name, size_t size, unsigned flags)
{
	if (size > MAX_ALLOC_SIZE)
		panic("%s: don't use pools for large allocations, use the VM"
		      " allocation interfaces instead.", __func__);
	kcprintf("POOL: Created dynamic pool \"%s\" of size %zu\n", name, size);
	spinlock_init(&pool->pool_lock, "DYNAMIC POOL");
	pool->pool_name = name;
	pool->pool_size = size;
	SLIST_INIT(&pool->pool_pages);
	pool->pool_flags = flags | POOL_VALID;
	return (0);
}

int
pool_destroy(struct pool *pool)
{
	panic("%s: can't destroy pools yet.", __func__);
	return (ERROR_NOT_IMPLEMENTED);
}

static struct pool_item *
pool_get(struct pool *pool)
{
	struct pool_page *page;
	struct pool_item *item;
	unsigned i;

	SLIST_FOREACH(page, &pool->pool_pages, pp_link) {
		if (page->pp_items == page->pp_maxitems)
			continue;
		for (i = 0; i < page->pp_maxitems; i++) {
			item = pool_page_item(page, i);
			ASSERT(pool_page(item) == page,
			       "item is within its page");
			if ((item->pi_flags & POOL_ITEM_FREE) != 0) {
				item->pi_flags ^= POOL_ITEM_FREE;
				page->pp_items++;
				return (item);
			}
		}
	}
	return (NULL);
}

static void
pool_initialize_page(struct pool_page *page)
{
	struct pool_item *item;
	unsigned i;

	for (i = 0; i < page->pp_maxitems; i++) {
		item = pool_page_item(page, i);
		item->pi_flags = POOL_ITEM_DEFAULT;
		item->pi_flags |= POOL_ITEM_FREE;
	}
}

static struct pool_page *
pool_page(struct pool_item *item)
{
	struct pool_page *page;

	page = (struct pool_page *)PAGE_FLOOR((uintptr_t)item);
	ASSERT(page->pp_magic == POOL_PAGE_MAGIC, "item in invalid page");
	return (page);
}

static struct pool_item *
pool_page_item(struct pool_page *page, unsigned i)
{
	struct pool_item *item;
	unsigned offset;

	ASSERT(i < page->pp_maxitems, "access past end of page.");
	offset = i * (page->pp_pool->pool_size + sizeof *item);
	item = (struct pool_item *)((uintptr_t)(page + 1) + offset);
	ASSERT(pool_page(item) == page, "item is within its page");
	return (item);
}

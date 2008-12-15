#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#ifdef DB
#include <db/db_show.h>
#endif
#include <io/console/console.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

#ifdef DB
DB_SHOW_TREE(pool, pool);
DB_SHOW_VALUE_TREE(pool, root, DB_SHOW_TREE_POINTER(pool));
#endif

#define	POOL_LOCK(pool)		spinlock_lock(&(pool)->pool_lock)
#define	POOL_UNLOCK(pool)	spinlock_unlock(&(pool)->pool_lock)

#define	POOL_ITEM_DEFAULT	(0x0000)
#define	POOL_ITEM_FREE		(0x0001)

struct pool_item {
	uint8_t pi_flags;
};

#define	POOL_PAGE_MAGIC		(0x19800604)
struct pool_page {
	uint32_t pp_magic;
	struct pool *pp_pool;
	SLIST_ENTRY(struct pool_page) pp_link;
	size_t pp_items;
};

#define	POOL_ITEM_OVERHEAD(n)						\
	ROUNDUP(((n) * sizeof (struct pool_item)), sizeof (uintmax_t))
#define	POOL_UTILIZATION(n, s)						\
	(sizeof (struct pool_page) + POOL_ITEM_OVERHEAD((n)) + ((s) * (n)))

#define	MAX_ALLOC_SIZE							\
	((PAGE_SIZE / 2) - (POOL_ITEM_OVERHEAD(2) + sizeof (struct pool_page)))

#ifdef DB
static TAILQ_HEAD(, struct pool) pool_list = TAILQ_HEAD_INITIALIZER(pool_list);
#endif

static struct pool_page *pool_datum_page(void *);
static void *pool_get(struct pool *);
static void pool_insert_page(struct pool *, struct pool_page *);
static struct pool_page *pool_item_page(struct pool_item *);
static void *pool_page_datum(struct pool_page *, unsigned);
static struct pool_item *pool_page_datum_item(struct pool_page *, void *);
static struct pool_item *pool_page_item(struct pool_page *, unsigned);

size_t pool_max_alloc = MAX_ALLOC_SIZE;

void *
pool_allocate(struct pool *pool)
{
	struct pool_item *item;
	vaddr_t vaddr;
	int error;

	if ((pool->pool_flags & POOL_VALID) == 0)
		panic("%s: pool %s used before initialization!", __func__,
		      pool->pool_name);
	ASSERT(pool->pool_size <= MAX_ALLOC_SIZE, "pool must not be so big.");

	POOL_LOCK(pool);
	item = pool_get(pool);
	if (item != NULL) {
		POOL_UNLOCK(pool);
		return ((void *)item);
	}

	if ((pool->pool_flags & POOL_MANAGED) != 0)
		panic("%s: pool %s exhausted.", __func__,
		      pool->pool_name);

	if ((pool->pool_flags & POOL_VIRTUAL) != 0) {
		error = vm_alloc_page(&kernel_vm, &vaddr);
	} else {
		error = page_alloc_direct(&kernel_vm, PAGE_FLAG_DEFAULT, &vaddr);
	}
	if (error != 0)
		panic("%s: can't map page for allocation: %m", __func__, error);

	pool_insert_page(pool, (struct pool_page *)vaddr);

	item = pool_get(pool);
	if (item == NULL)
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
	vaddr_t vaddr;
	int error;

	page = pool_datum_page(m);
	item = pool_page_datum_item(page, m);
	ASSERT(page == pool_item_page(item), "datum and item must match page");
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
	 * page itself back into the free pool, unless this is a managed pool.
	 */
	if ((pool->pool_flags & POOL_MANAGED) == 0) {
		page->pp_magic = ~POOL_PAGE_MAGIC;
		SLIST_REMOVE(&pool->pool_pages, page, struct pool_page,
			     pp_link);
		vaddr = (vaddr_t)page;
		if ((pool->pool_flags & POOL_VIRTUAL) != 0) {
			error = vm_free_page(&kernel_vm, vaddr);
		} else {
			error = page_free_direct(&kernel_vm, vaddr);
		}
		if (error != 0)
			panic("%s: can't free page: %m", __func__, error);
	}
	POOL_UNLOCK(pool);
}

int
pool_create(struct pool *pool, const char *name, size_t size, unsigned flags)
{
	if (size > MAX_ALLOC_SIZE)
		panic("%s: don't use pool %s for large allocations, use the VM"
		      " allocation interfaces instead.", __func__, name);
	if ((flags & (POOL_VIRTUAL | POOL_MANAGED)) ==
	    (POOL_VIRTUAL | POOL_MANAGED))
		panic("%s: what you do with managed pool %s is your business.",
		      __func__, name);
	spinlock_init(&pool->pool_lock, "DYNAMIC POOL", SPINLOCK_FLAG_DEFAULT);
	pool->pool_name = name;
	pool->pool_size = size;
	/*
	 * This can be done constantly, but I'm too tired at the moment to work
	 * it out.
	 */
	for (pool->pool_maxitems = 0;
	     POOL_UTILIZATION(pool->pool_maxitems + 1, size) <= PAGE_SIZE;
	     pool->pool_maxitems++)
		continue;
	SLIST_INIT(&pool->pool_pages);
	pool->pool_flags = flags | POOL_VALID;
#ifdef VERBOSE
	kcprintf("POOL: Created pool \"%s\" of size %zu (%zu/pg)\n",
		 pool->pool_name, pool->pool_size, pool->pool_maxitems);
#endif
#ifdef DB
	TAILQ_INSERT_TAIL(&pool_list, pool, pool_link);
#endif
	return (0);
}

void
pool_insert(struct pool *pool, vaddr_t vaddr)
{
	struct pool_page *page;

	POOL_LOCK(pool);
	if ((pool->pool_flags & POOL_MANAGED) == 0)
		panic("%s: keep your hands off %s.", __func__, pool->pool_name);
	page = (struct pool_page *)vaddr;
	pool_insert_page(pool, page);
	POOL_UNLOCK(pool);
}

static struct pool_page *
pool_datum_page(void *datum)
{
	struct pool_page *page;

	page = (struct pool_page *)PAGE_FLOOR((uintptr_t)datum);
	ASSERT(page->pp_magic == POOL_PAGE_MAGIC, "datum in invalid page");
	return (page);
}

int
pool_destroy(struct pool *pool)
{
	panic("%s: can't destroy pools yet.", __func__);
}

static void *
pool_get(struct pool *pool)
{
	struct pool_page *page;
	struct pool_item *item;
	unsigned i;

	SLIST_FOREACH(page, &pool->pool_pages, pp_link) {
		if (page->pp_items == pool->pool_maxitems)
			continue;
		for (i = 0; i < pool->pool_maxitems; i++) {
			item = pool_page_item(page, i);
			ASSERT(pool_item_page(item) == page,
			       "item is within its page");
			if ((item->pi_flags & POOL_ITEM_FREE) != 0) {
				item->pi_flags ^= POOL_ITEM_FREE;
				page->pp_items++;
				return (pool_page_datum(page, i));
			}
		}
	}
	return (NULL);
}

static void
pool_insert_page(struct pool *pool, struct pool_page *page)
{
	struct pool_item *item;
	unsigned i;

	page->pp_magic = POOL_PAGE_MAGIC;
	page->pp_pool = pool;
	SLIST_INSERT_HEAD(&pool->pool_pages, page, pp_link);
	page->pp_items = 0;

	for (i = 0; i < pool->pool_maxitems; i++) {
		item = pool_page_item(page, i);
		item->pi_flags = POOL_ITEM_DEFAULT;
		item->pi_flags |= POOL_ITEM_FREE;
	}
}

static struct pool_page *
pool_item_page(struct pool_item *item)
{
	struct pool_page *page;

	page = (struct pool_page *)PAGE_FLOOR((uintptr_t)item);
	ASSERT(page->pp_magic == POOL_PAGE_MAGIC, "item in invalid page");
	return (page);
}

static void *
pool_page_datum(struct pool_page *page, unsigned i)
{
	void *datum;
	unsigned offset;

	ASSERT(i < page->pp_pool->pool_maxitems, "access past end of page.");
	offset = POOL_ITEM_OVERHEAD(page->pp_pool->pool_maxitems) +
		i * page->pp_pool->pool_size;
	datum = (void *)((uintptr_t)(page + 1) + offset);
	ASSERT(pool_datum_page(datum) == page, "datum is within its page");
	return (datum);
}

static struct pool_item *
pool_page_datum_item(struct pool_page *page, void *datum)
{
	struct pool_item *item;
	unsigned i;

	i = ((uintptr_t)datum - (uintptr_t)pool_page_datum(page, 0)) /
		page->pp_pool->pool_size;
	ASSERT(i < page->pp_pool->pool_maxitems, "access past end of page.");
	item = pool_page_item(page, i);
	ASSERT(pool_item_page(item) == page, "item is within its page");
	return (item);
}

static struct pool_item *
pool_page_item(struct pool_page *page, unsigned i)
{
	struct pool_item *item;
	unsigned offset;

	ASSERT(i < page->pp_pool->pool_maxitems, "access past end of page.");
	offset = i * sizeof *item;
	item = (struct pool_item *)((uintptr_t)(page + 1) + offset);
	ASSERT(pool_item_page(item) == page, "item is within its page");
	return (item);
}

#ifdef DB
static void
db_pool_dump_pool(struct pool *pool, bool pages, bool items)
{
	struct pool_item *item;
	struct pool_page *page;
	unsigned i;

	kcprintf("pool %p %s size %zu maxitems %zu\n", pool, pool->pool_name,
		 pool->pool_size, pool->pool_maxitems);
	if (!pages)
		return;
	SLIST_FOREACH(page, &pool->pool_pages, pp_link) {
		kcprintf("\tpage %p items %zu\n", page, page->pp_items);
		if (!items)
			continue;
		for (i = 0; i < pool->pool_maxitems; i++) {
			item = pool_page_item(page, i);
			kcprintf("\t\titem %p flags %x\n", item,
				 item->pi_flags);
		}
	}
}

static void
db_pool_dump_all(void)
{
	struct pool *pool;

	TAILQ_FOREACH(pool, &pool_list, pool_link)
		db_pool_dump_pool(pool, true, true);
}
DB_SHOW_VALUE_VOIDF(all, pool, db_pool_dump_all);

static void
db_pool_dump_pages(void)
{
	struct pool *pool;

	TAILQ_FOREACH(pool, &pool_list, pool_link)
		db_pool_dump_pool(pool, true, false);
}
DB_SHOW_VALUE_VOIDF(pages, pool, db_pool_dump_pages);

static void
db_pool_dump_pools(void)
{
	struct pool *pool;

	TAILQ_FOREACH(pool, &pool_list, pool_link)
		db_pool_dump_pool(pool, false, false);
}
DB_SHOW_VALUE_VOIDF(pools, pool, db_pool_dump_pools);
#endif

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

typedef	uint64_t	pool_map_t;

#ifdef INVARIANTS
#define	POOL_PAGE_MAGIC		(0x19800604)
#endif

struct pool_page {
#ifdef INVARIANTS
	uint32_t pp_magic;
#endif
	struct pool *pp_pool;
	SLIST_ENTRY(struct pool_page) pp_link;
	size_t pp_items;
};

#define	POOL_ALIGNMENT		(8)

#define	POOL_MAP_WORD_BITS	(sizeof (pool_map_t) * 8)
#define	POOL_MAP_BITS(n)	(ROUNDUP((n), POOL_MAP_WORD_BITS))
#define	POOL_MAP_BYTES(n)	(POOL_MAP_BITS((n)) / 8)
#define	POOL_MAP_WORDS(n)	(POOL_MAP_BYTES((n)) / sizeof (pool_map_t))
#define	POOL_WORD(n)		((n) / POOL_MAP_WORD_BITS)
#define	POOL_OFFSET(n)		((n) % POOL_MAP_WORD_BITS)
#define	POOL_UTILIZATION(n, s)						\
	(sizeof (struct pool_page) + POOL_MAP_BYTES((n)) + ((s) * (n)))

#define	MAX_ALLOC_SIZE							\
	((PAGE_SIZE / 2) - (POOL_MAP_BYTES(2) + sizeof (struct pool_page)))

#ifdef DB
static TAILQ_HEAD(, struct pool) pool_list = TAILQ_HEAD_INITIALIZER(pool_list);
#endif

static struct pool_page *pool_datum_page(void *);
static void *pool_get(struct pool *);
static void pool_insert_page(struct pool *, struct pool_page *);
static void *pool_page_datum(struct pool_page *, unsigned);
static void pool_page_free_datum(struct pool_page *, void *);

size_t pool_max_alloc = MAX_ALLOC_SIZE;

void *
pool_allocate(struct pool *pool)
{
	vaddr_t vaddr;
	void *datum;
	int error;

	if ((pool->pool_flags & POOL_VALID) == 0)
		panic("%s: pool %s used before initialization!", __func__,
		      pool->pool_name);
	ASSERT(pool->pool_size <= MAX_ALLOC_SIZE, "pool must not be so big.");

	POOL_LOCK(pool);
	datum = pool_get(pool);
	if (datum != NULL) {
		POOL_UNLOCK(pool);
		return (datum);
	}

	if ((pool->pool_flags & POOL_VIRTUAL) != 0) {
		error = vm_alloc_page(&kernel_vm, &vaddr);
	} else {
		error = page_alloc_direct(&kernel_vm, PAGE_FLAG_DEFAULT, &vaddr);
	}
	if (error != 0)
		panic("%s: can't map page for allocation: %m", __func__, error);

	pool_insert_page(pool, (struct pool_page *)vaddr);

	datum = pool_get(pool);
	if (datum == NULL)
		panic("%s: can't get memory but we just allocated!", __func__);
	POOL_UNLOCK(pool);
	return (datum);
}

void
pool_free(void *m)
{
	struct pool_page *page;
	struct pool *pool;
	vaddr_t vaddr;
	int error;

	page = pool_datum_page(m);
	pool = page->pp_pool;
	ASSERT((pool->pool_flags & POOL_VALID) != 0, "pool must be valid.");
	POOL_LOCK(pool);
	pool_page_free_datum(page, m);
	if (page->pp_items == 0)
		panic("%s: pool %s has no items.", __func__, pool->pool_name);
	if (page->pp_items-- != 1) {
		POOL_UNLOCK(pool);
		return;
	}
	/*
	 * Last datum in page, unmap it, free any virtual addresses and put the
	 * page itself back into the free pool.
	 */
#ifdef INVARIANTS
	page->pp_magic = ~POOL_PAGE_MAGIC;
#endif
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

	POOL_UNLOCK(pool);
}

int
pool_create(struct pool *pool, const char *name, size_t size, unsigned flags)
{
#ifdef VERBOSE
	if ((size % POOL_ALIGNMENT) != 0) {
		kcprintf("POOL: Rounding up pool \"%s\" from %zu to %zu\n",
			 size, ROUNDUP(size, POOL_ALIGNMENT));
	}
#endif
	size = ROUNDUP(size, POOL_ALIGNMENT);

	if (size > MAX_ALLOC_SIZE)
		panic("%s: don't use pool %s for large allocations, use the VM"
		      " allocation interfaces instead.", __func__, name);

	spinlock_init(&pool->pool_lock, "DYNAMIC POOL", SPINLOCK_FLAG_DEFAULT);
	pool->pool_name = name;
	pool->pool_size = size;
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

static struct pool_page *
pool_datum_page(void *datum)
{
	struct pool_page *page;

	page = (struct pool_page *)PAGE_FLOOR((uintptr_t)datum);
#ifdef INVARIANTS
	ASSERT(page->pp_magic == POOL_PAGE_MAGIC, "datum in invalid page");
#endif
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
	pool_map_t *map;
	unsigned i, j, o;

	SLIST_FOREACH(page, &pool->pool_pages, pp_link) {
		if (page->pp_items == pool->pool_maxitems)
			continue;
		map = (pool_map_t *)(void *)(page + 1);
		for (i = 0; i < POOL_MAP_WORDS(pool->pool_maxitems); i++) {
			if (map[i] == (pool_map_t)0)
				continue;
			o = i * POOL_MAP_WORD_BITS;
			for (j = 0; o + j < pool->pool_maxitems &&
			     j < POOL_MAP_WORD_BITS; j++) {
				if ((map[i] & (1ull << j)) != 0) {
					map[i] &= ~(1ull << j);
					page->pp_items++;
					return (pool_page_datum(page, o + j));
				}
			}
			/*
			 * XXX
			 * This should not be reached unless this word contains
			 * some bits that will never ever be set.
			 */
		}
		NOTREACHED();
	}
	return (NULL);
}

static void
pool_insert_page(struct pool *pool, struct pool_page *page)
{
	pool_map_t *map = (pool_map_t *)(void *)(page + 1);
	unsigned i, o;

#ifdef INVARIANTS
	page->pp_magic = POOL_PAGE_MAGIC;
#endif
	page->pp_pool = pool;
	SLIST_INSERT_HEAD(&pool->pool_pages, page, pp_link);
	page->pp_items = 0;

	/*
	 * Set all bits in N-1 words, where N is the number of words.  Then
	 * set only the remaining bits in the final word.
	 */
	for (i = 0; i < POOL_MAP_WORDS(pool->pool_maxitems) - 1; i++) {
		map[i] = ~(pool_map_t)0;
	}

	o = POOL_OFFSET(pool->pool_maxitems - 1);
	if (o == POOL_MAP_WORD_BITS - 1) {
		map[i] = ~(pool_map_t)0;
	} else {
		map[i] = (1ull << (o + 1)) - 1;
	}
}

static void *
pool_page_datum(struct pool_page *page, unsigned i)
{
	void *datum;
	unsigned offset;

	ASSERT(i < page->pp_pool->pool_maxitems, "access past end of page.");
	offset = POOL_MAP_BYTES(page->pp_pool->pool_maxitems) +
		i * page->pp_pool->pool_size;
	datum = (void *)((uintptr_t)(page + 1) + offset);
	ASSERT(pool_datum_page(datum) == page, "datum is within its page");
	return (datum);
}

static void
pool_page_free_datum(struct pool_page *page, void *datum)
{
	pool_map_t *map = (pool_map_t *)(void *)(page + 1);
	unsigned i;

#ifdef INVARIANTS
	ASSERT(page->pp_magic == POOL_PAGE_MAGIC, "datum in invalid page");
#endif
	i = ((uintptr_t)datum - (uintptr_t)pool_page_datum(page, 0)) /
		page->pp_pool->pool_size;
	map[POOL_WORD(i)] |= (1ull << POOL_OFFSET(i));
}

#ifdef DB
static void
db_pool_dump_pool(struct pool *pool, bool pages, bool items)
{
	struct pool_page *page;

	kcprintf("pool %p %s size %zu maxitems %zu\n", pool, pool->pool_name,
		 pool->pool_size, pool->pool_maxitems);
	if (!pages)
		return;
	SLIST_FOREACH(page, &pool->pool_pages, pp_link) {
		kcprintf("\tpage %p items %zu\n", page, page->pp_items);
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

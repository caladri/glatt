#include <core/types.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/startup.h>

struct malloc_bucket {
	struct pool mb_pool;
	size_t mb_size;
};

#define	MALLOC_NBUCKETS	(7)

static struct malloc_bucket malloc_buckets[MALLOC_NBUCKETS];
static size_t malloc_biggest;
static struct pool malloc_bigbucket;

static struct pool *malloc_pool(size_t);

void
free(void *p)
{
	pool_free(p);
}

void *
malloc(size_t size)
{
	struct pool *pool;
	void *p;

	pool = malloc_pool(size);
	p = pool_allocate(pool);
	return (p);
}

static struct pool *
malloc_pool(size_t size)
{
	struct malloc_bucket *mb;
	unsigned i;

	ASSERT(size <= pool_max_alloc, "Don't be silly.");

	for (i = 0; i < MALLOC_NBUCKETS; i++) {
		mb = &malloc_buckets[i];
		if (mb->mb_size >= size)
			return (&mb->mb_pool);
	}
	return (&malloc_bigbucket);
}

static void
malloc_setup(void *arg)
{
	struct malloc_bucket *mb;
	unsigned i;
	int error;
	size_t j;

	j = 16;

	for (i = 0; i < MALLOC_NBUCKETS; i++) {
		mb = &malloc_buckets[i];
		error = pool_create(&mb->mb_pool, "MALLOC", j, POOL_VIRTUAL);
		if (error != 0)
			panic("%s: pool_create failed: %m", __func__, error);
		mb->mb_size = j;
		j *= 2;
	}
	error = pool_create(&malloc_bigbucket, "MALLOC BIG", pool_max_alloc,
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}
STARTUP_ITEM(malloc, STARTUP_POOL, STARTUP_FIRST, malloc_setup, NULL);

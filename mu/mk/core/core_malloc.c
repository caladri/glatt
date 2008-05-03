#include <core/types.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/startup.h>

struct malloc_bucket {
	struct pool mb_pool;
	size_t mb_size;
};

#define	MALLOC_NBUCKETS	(8)

static struct malloc_bucket malloc_buckets[MALLOC_NBUCKETS];
static struct malloc_bucket malloc_bigbucket;

static struct malloc_bucket *malloc_bucket(size_t);

void
free(void *p)
{
	pool_free(p);
}

void *
malloc(size_t size)
{
	struct malloc_bucket *mb;
	void *p;

	mb = malloc_bucket(size);
	p = pool_allocate(&mb->mb_pool);
	return (p);
}

static struct malloc_bucket *
malloc_bucket(size_t size)
{
	struct malloc_bucket *mb;
	unsigned i;

	ASSERT(size <= pool_max_alloc, "Don't be silly.");

	for (i = 0; i < MALLOC_NBUCKETS; i++) {
		mb = &malloc_buckets[i];
		if (mb->mb_size >= size)
			return (mb);
	}
	return (&malloc_bigbucket);
}

static void
malloc_setup_bucket(struct malloc_bucket *mb, size_t size)
{
	int error;

	error = pool_create(&mb->mb_pool, mb == &malloc_bigbucket ?
			    "MALLOC BIG" : "MALLOC", size, POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	mb->mb_size = size;
}

static void
malloc_setup(void *arg)
{
	struct malloc_bucket *mb;
	unsigned i;
	size_t j;

	j = 16;

	for (i = 0; i < MALLOC_NBUCKETS; i++) {
		mb = &malloc_buckets[i];
		malloc_setup_bucket(mb, j);
		j *= 2;
	}
	malloc_setup_bucket(&malloc_bigbucket, pool_max_alloc);
}
STARTUP_ITEM(malloc, STARTUP_POOL, STARTUP_FIRST, malloc_setup, NULL);

#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>

void *
pool_allocate(struct pool *pool)
{
	return (NULL);
}

void
pool_free(struct pool *pool, void *m)
{
}

int
pool_create(struct pool **poolp, size_t size, unsigned flags)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
pool_destroy(struct pool *pool)
{
	return (ERROR_NOT_IMPLEMENTED);
}

#ifndef	_CORE_POOL_H_
#define	_CORE_POOL_H_

#include <core/queue.h>

struct pool_page;

#define	POOL_DEFAULT	(0x00000000)	/* Default pool flags.  */
#define	POOL_BLOCKING	(0x00000001)	/* Pool allocations may block.  */
#define	POOL_VIRTUAL	(0x00000002)	/* Map virtual, not direct-map.  */

struct pool {
	const char *pool_name;
	size_t pool_size;
	SLIST_HEAD(, struct pool_page) pool_pages;
	uint32_t pool_flags;
};

#define	POOL_INIT(name, type, flags)					\
	{								\
		.pool_name = name,					\
		.pool_size = sizeof (type),				\
		.pool_flags = flags,					\
		.pool_pages = SLIST_HEAD_INITIALIZER()			\
	}

extern size_t pool_max_alloc;

void *pool_allocate(struct pool *);
int pool_create(struct pool *, const char *, size_t, unsigned);
void pool_free(struct pool *, void *);
int pool_destroy(struct pool *);

#endif /* _CORE_POOL_H_ */

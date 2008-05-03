#ifndef	_CORE_POOL_H_
#define	_CORE_POOL_H_

#include <core/queue.h>
#include <core/spinlock.h>

struct pool_page;

#define	POOL_DEFAULT	(0x00000000)	/* Default pool flags.  */
#define	POOL_BLOCKING	(0x00000001)	/* Pool allocations may block.  */
#define	POOL_VIRTUAL	(0x00000002)	/* Map virtual, not direct-map.  */
#define	POOL_VALID	(0x00000004)	/* Pool is valid.  */

struct pool {
	struct spinlock pool_lock;
	const char *pool_name;
	size_t pool_size;
	size_t pool_maxitems;
	SLIST_HEAD(, struct pool_page) pool_pages;
	uint32_t pool_flags;
	TAILQ_ENTRY(struct pool) pool_link;
};

extern size_t pool_max_alloc;

void *pool_allocate(struct pool *);
int pool_create(struct pool *, const char *, size_t, unsigned);
void pool_free(void *);
int pool_destroy(struct pool *);

#endif /* _CORE_POOL_H_ */

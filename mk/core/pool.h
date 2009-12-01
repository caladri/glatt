#ifndef	_CORE_POOL_H_
#define	_CORE_POOL_H_

#include <core/queue.h>
#include <core/spinlock.h>

struct pool_page;

#define	POOL_DEFAULT	(0x00000000)	/* Default pool flags.  */
#define	POOL_VIRTUAL	(0x00000001)	/* Map virtual, not direct-map.  */
#define	POOL_VALID	(0x00000002)	/* Pool is valid.  */

struct pool {
	struct spinlock pool_lock;
	const char *pool_name;
	size_t pool_size;
	size_t pool_maxitems;
	size_t pool_freeitems;
	SLIST_HEAD(, struct pool_page) pool_pages;
	unsigned pool_flags;
#ifdef DB
	TAILQ_ENTRY(struct pool) pool_link;
#endif
};

extern size_t pool_max_alloc;

void *pool_allocate(struct pool *) __malloc __non_null(1);
int pool_create(struct pool *, const char *, size_t, unsigned) __non_null(1, 2);
void pool_free(void *) __non_null(1);
void pool_insert(struct pool *, vaddr_t) __non_null(1);
int pool_destroy(struct pool *) __non_null(1);

#endif /* _CORE_POOL_H_ */

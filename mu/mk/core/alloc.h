#ifndef	_CORE_ALLOC_H_
#define	_CORE_ALLOC_H_

struct pool;

#define	POOL_DEFAULT	(0x00000000)	/* Default pool flags.  */
#define	POOL_BLOCKING	(0x00000001)	/* Pool allocations may block.  */

void *pool_allocate(struct pool *);
int pool_create(struct pool **, size_t, unsigned);
void pool_free(struct pool *, void *);
int pool_destroy(struct pool *);

#endif /* _CORE_ALLOC_H_ */

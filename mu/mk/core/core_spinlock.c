#include <core/types.h>
#include <core/pool.h>
#include <core/spinlock.h>

static struct pool spinlock_pool = POOL_INIT("SPINLOCK", struct spinlock, POOL_DEFAULT);

struct spinlock *
spinlock_allocate(const char *name)
{
	struct spinlock *lock;

	lock = pool_allocate(&spinlock_pool);
	spinlock_init(lock, name);
	return (lock);
}

void
spinlock_init(struct spinlock *lock, const char *name)
{
	critical_section_t crit;

	crit = critical_enter();
	lock->s_name = name;
	atomic_store_64(&lock->s_owner, CPU_ID_INVALID);
	atomic_store_64(&lock->s_nest, 0);
	critical_exit(crit);
}

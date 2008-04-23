#include <core/types.h>
#include <core/spinlock.h>

void
spinlock_init(struct spinlock *lock, const char *name, unsigned flags)
{
	critical_section_t crit;

	crit = critical_enter();
	lock->s_name = name;
	atomic_store_64(&lock->s_owner, CPU_ID_INVALID);
	atomic_store_64(&lock->s_nest, 0);
	lock->s_flags = flags;
	critical_exit(crit);
}

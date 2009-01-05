#include <core/types.h>
#include <core/spinlock.h>

void
spinlock_init(struct spinlock *lock, const char *name, unsigned flags)
{
	critical_section_t crit;

	crit = critical_enter();
	lock->s_name = name;
	atomic_store64(&lock->s_owner, CPU_ID_INVALID);
	atomic_store64(&lock->s_nest, 0);
	lock->s_flags = flags;
	critical_exit(crit);
}

void
spinlock_lock(struct spinlock *lock)
{
#ifndef	UNIPROCESSOR
	critical_section_t crit;

	crit = critical_enter();
	while (!atomic_cmpset64(&lock->s_owner, CPU_ID_INVALID, mp_whoami())) {
		if (atomic_load64(&lock->s_owner) == (uint64_t)mp_whoami()) {
			if ((lock->s_flags & SPINLOCK_FLAG_RECURSE) != 0) {
				atomic_increment64(&lock->s_nest);
				critical_exit(crit);
				return;
			}
			panic("%s: cannot recurse on spinlock (%s)", __func__,
			      lock->s_name);
		}
		critical_exit(crit);
		/*
		 * If we are not already in a critical section, allow interrupts
		 * to fire while we spin.
		 */
		crit = critical_enter();
	}
	lock->s_crit = crit;
#else
	critical_section_t crit;

	crit = critical_enter();
	if (lock->s_owner == mp_whoami()) {
		lock->s_nest++;
		critical_exit(crit);
	} else {
		lock->s_crit = crit;
		lock->s_owner = mp_whoami();
	}
#endif
}

void
spinlock_unlock(struct spinlock *lock)
{
#ifndef	UNIPROCESSOR
	critical_section_t crit;
	critical_section_t saved;

	crit = critical_enter();
	if (atomic_load64(&lock->s_owner) == (uint64_t)mp_whoami()) {
		if (atomic_load64(&lock->s_nest) != 0) {
			atomic_decrement64(&lock->s_nest);
			critical_exit(crit);
			return;
		} else {
			saved = lock->s_crit;
			if (atomic_cmpset64(&lock->s_owner, mp_whoami(),
					    CPU_ID_INVALID)) {
				critical_exit(crit);
				critical_exit(saved);
				return;
			}
		}
	}
	critical_exit(crit);
	panic("%s: not my lock to unlock (%s)", __func__, lock->s_name);
#else
	if (lock->s_owner != mp_whoami())
		panic("%s: cannot lock %s (owner=%lx)", __func__,
		      lock->s_name, lock->s_owner);

	if (lock->s_nest == 0) {
		critical_exit(lock->s_crit);
		lock->s_owner = CPU_ID_INVALID;
	} else
		lock->s_nest--;
#endif
}

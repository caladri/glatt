#include <core/types.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>

void
spinlock_init(struct spinlock *lock, const char *name, unsigned flags)
{
	ASSERT((flags & SPINLOCK_FLAG_VALID) == 0, "Must not set valid flag.");
	lock->s_name = name;
	lock->s_owner = CPU_ID_INVALID;
	lock->s_nest = 0;
	lock->s_flags = flags | SPINLOCK_FLAG_VALID;
}

void
spinlock_lock(struct spinlock *lock)
{
	ASSERT((lock->s_flags & SPINLOCK_FLAG_VALID) != 0,
	       "Cowardly refusing to lock invalid spinlock.");

	if (startup_early)
		return;
#ifndef	UNIPROCESSOR
	critical_enter();
	cpu_id_t self = mp_whoami();
	while (!atomic_cmpset64(&lock->s_owner, CPU_ID_INVALID, self)) {
		if (atomic_load64(&lock->s_owner) == (uint64_t)self) {
			if ((lock->s_flags & SPINLOCK_FLAG_RECURSE) != 0) {
				atomic_increment64(&lock->s_nest);
				critical_exit();
				return;
			}
			panic("%s: cannot recurse on spinlock (%s)", __func__,
			      lock->s_name);
		}
		critical_exit();
		/*
		 * If we are not already in a critical section, allow interrupts
		 * to fire while we spin.
		 */
		critical_enter();
		self = mp_whoami();
	}
#else
	critical_enter();
	if (lock->s_owner == mp_whoami()) {
		lock->s_nest++;
		critical_exit();
	} else {
		lock->s_owner = mp_whoami();
	}
#endif
}

void
spinlock_unlock(struct spinlock *lock)
{
	ASSERT((lock->s_flags & SPINLOCK_FLAG_VALID) != 0,
	       "Cowardly refusing to unlock invalid spinlock.");

	if (startup_early)
		return;
#ifndef	UNIPROCESSOR
	cpu_id_t self = mp_whoami();
	if (atomic_load64(&lock->s_owner) == (uint64_t)self) {
		if (atomic_load64(&lock->s_nest) != 0) {
			atomic_decrement64(&lock->s_nest);
			return;
		} else {
			if (atomic_cmpset64(&lock->s_owner, self,
					    CPU_ID_INVALID)) {
				critical_exit();
				return;
			}
		}
	}
	panic("%s: not my lock to unlock (%s)", __func__, lock->s_name);
#else
	if (lock->s_owner != mp_whoami())
		panic("%s: cannot lock %s (owner=%lx)", __func__,
		      lock->s_name, lock->s_owner);

	if (lock->s_nest == 0) {
		critical_exit();
		lock->s_owner = CPU_ID_INVALID;
	} else
		lock->s_nest--;
#endif
}

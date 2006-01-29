#ifndef	_CORE_SPINLOCK_H_
#define	_CORE_SPINLOCK_H_

#include <core/atomic.h>
#include <core/critical.h>
#include <core/mp.h>

struct spinlock {
	const char *s_name;
	uint64_t s_owner;
	uint64_t s_nest;
};

static inline void
spinlock_lock(struct spinlock *lock)
{
	critical_section_t crit;

	crit = critical_enter();
	while (!atomic_compare_and_set_64(&lock->s_owner, 0, mp_whoami() + 1)) {
		if (atomic_load_64(&lock->s_owner) ==
		    (uint64_t)(mp_whoami() + 1)) {
			atomic_increment_64(&lock->s_nest);
			critical_exit(crit);
			return;
		}
		critical_exit(crit);
		crit = critical_enter();
	}
	critical_exit(crit);
}

static inline void
spinlock_unlock(struct spinlock *lock)
{
	critical_section_t crit;

	crit = critical_enter();
	if (atomic_load_64(&lock->s_owner) == (uint64_t)(mp_whoami() + 1)) {
		if (atomic_load_64(&lock->s_nest) != 0) {
			atomic_decrement_64(&lock->s_nest);
			critical_exit(crit);
			return;
		} else {
			if (atomic_compare_and_set_64(&lock->s_owner,
						      mp_whoami() + 1, 0)) {
				critical_exit(crit);
				return;
			}
		}
	}
	critical_exit(crit);
	/* XXX panic.  */
}

void spinlock_init(struct spinlock *, const char *);

#endif /* !_CORE_SPINLOCK_H_ */

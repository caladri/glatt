#ifndef	_CORE_SPINLOCK_H_
#define	_CORE_SPINLOCK_H_

#include <core/clock.h>
#include <core/critical.h>
#include <core/mp.h>
#include <cpu/atomic.h>
#include <db/db.h>

struct spinlock {
	const char *s_name;
	uint64_t s_owner;
	uint64_t s_nest;
	critical_section_t s_crit;
};

#define	SPINLOCK_INIT(name)						\
	{								\
		.s_name = name,						\
		.s_owner = CPU_ID_INVALID,				\
	}

static inline void
spinlock_lock(struct spinlock *lock)
{
#ifndef	UNIPROCESSOR
	critical_section_t crit;
	clock_ticks_t ticks;

	ticks = clock();

	crit = critical_enter();
	while (!atomic_compare_and_set_64(&lock->s_owner, CPU_ID_INVALID,
					  mp_whoami())) {
		if (atomic_load_64(&lock->s_owner) == (uint64_t)mp_whoami()) {
			atomic_increment_64(&lock->s_nest);
			critical_exit(crit);
			return;
		}
		critical_exit(crit);
		if (clock() - ticks > 100)
			panic("Spinlock \"%s\" spun for 100 ticks.\n",
			      lock->s_name);
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

static inline void
spinlock_unlock(struct spinlock *lock)
{
#ifndef	UNIPROCESSOR
	critical_section_t crit;
	critical_section_t saved;

	crit = critical_enter();
	if (atomic_load_64(&lock->s_owner) == (uint64_t)mp_whoami()) {
		if (atomic_load_64(&lock->s_nest) != 0) {
			atomic_decrement_64(&lock->s_nest);
			critical_exit(crit);
			return;
		} else {
			saved = lock->s_crit;
			if (atomic_compare_and_set_64(&lock->s_owner,
						      mp_whoami(),
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

#define	SPINLOCK_ASSERT_HELD(lock)					\
	ASSERT(atomic_load_64(&(lock)->s_owner) == (uint64_t)mp_whoami(),\
	       "Lock must be held.")

struct spinlock *spinlock_allocate(const char *);
void spinlock_init(struct spinlock *, const char *);

#endif /* !_CORE_SPINLOCK_H_ */

#ifndef	_CORE_SPINLOCK_H_
#define	_CORE_SPINLOCK_H_

#include <core/critical.h>
#include <core/mp.h>
#include <cpu/atomic.h>

struct spinlock {
	const char *s_name;
	uint64_t s_owner;
	uint64_t s_nest;
	unsigned s_flags;
	critical_section_t s_crit;
};

#define	SPINLOCK_FLAG_DEFAULT	(0x00000000)
#define	SPINLOCK_FLAG_RECURSE	(0x00000001)

#define	SPINLOCK_ASSERT_HELD(lock)					\
	ASSERT(atomic_load_64(&(lock)->s_owner) == (uint64_t)mp_whoami(),\
	       "Lock must be held.")

void spinlock_init(struct spinlock *, const char *, unsigned);
void spinlock_lock(struct spinlock *);
void spinlock_unlock(struct spinlock *);

#endif /* !_CORE_SPINLOCK_H_ */

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
};

#define	SPINLOCK_FLAG_DEFAULT	(0x00000000)
#define	SPINLOCK_FLAG_RECURSE	(0x00000001)
#define	SPINLOCK_FLAG_VALID	(0x00000002)

#define	SPINLOCK_ASSERT_HELD(lock)					\
	ASSERT(atomic_load64(&(lock)->s_owner) == (uint64_t)mp_whoami(),\
	       "Lock must be held.")

void spinlock_init(struct spinlock *, const char *, unsigned) __non_null(1, 2);
void spinlock_lock(struct spinlock *) __non_null(1);
void spinlock_unlock(struct spinlock *) __non_null(1);

#endif /* !_CORE_SPINLOCK_H_ */

#ifndef	_CORE_MUTEX_H_
#define	_CORE_MUTEX_H_

#include <core/sleepq.h>
#include <core/spinlock.h>

struct thread;

struct mutex {
	struct spinlock mtx_lock;
	struct sleepq mtx_sleepq;
	struct thread *mtx_owner;
	unsigned mtx_nested;
	unsigned mtx_waiters;
	unsigned mtx_flags;
};

#define	MUTEX_FLAG_DEFAULT	(0x00000000)
#define	MUTEX_FLAG_RECURSE	(0x00000001)

#define	MUTEX_HELD(mtx)							\
	((mtx)->mtx_owner != NULL && (mtx)->mtx_owner == current_thread())

void mutex_init(struct mutex *, const char *, unsigned) __non_null(1, 2);
void mutex_lock(struct mutex *) __non_null(1);
void mutex_unlock(struct mutex *) __non_null(1);

#endif /* !_CORE_MUTEX_H_ */

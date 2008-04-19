#ifndef	_CORE_MORDER_H_
#define	_CORE_MORDER_H_

#include <core/queue.h>

struct morder_lock;
struct mutex;
struct thread;

struct morder_thread {
	SLIST_HEAD(, struct morder_lock) mt_locks;
};

void morder_init(void);
void morder_thread_setup(struct thread *);

void morder_lock(struct mutex *);
void morder_unlock(struct mutex *);

#endif /* !_CORE_MORDER_H_ */

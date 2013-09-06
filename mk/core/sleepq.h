#ifndef	_CORE_SLEEPQ_H_
#define	_CORE_SLEEPQ_H_

#include <core/queue.h>

struct sleepq_entry;
struct spinlock;

struct sleepq {
	struct spinlock *sq_lock;
	TAILQ_HEAD(, struct sleepq_entry) sq_entries;
};

void sleepq_init(struct sleepq *, struct spinlock *);
void sleepq_enter(struct sleepq *);
void sleepq_signal(struct sleepq *);
void sleepq_signal_one(struct sleepq *);

#endif /* !_CORE_SLEEPQ_H_ */

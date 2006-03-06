#include <core/types.h>
#include <core/alloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <io/device/console/console.h>

struct startup_item_sorted {
	struct startup_item *sis_item;
	struct startup_item_sorted *sis_prev;
	struct startup_item_sorted *sis_next;
};

SET(startup_items, struct startup_item);

static bool startup_booting = false;
static struct spinlock startup_spinlock = SPINLOCK_INIT("startup");
static struct task *main_task;

void
startup_boot(void)
{
	struct startup_item **itemp, *item;
	struct startup_item_sorted *first, *sorted, *ip;
	struct pool startup_item_pool;
	int error;

	kcprintf("The system is coming up.\n");

	error = pool_create(&startup_item_pool, "startup item", sizeof *sorted,
			    POOL_VIRTUAL);
	first = NULL;
	for (itemp = SET_BEGIN(startup_items); itemp < SET_END(startup_items);
	     itemp++) {
		item = *itemp;
		sorted = pool_allocate(&startup_item_pool);
		ASSERT(sorted != NULL, "allocation failed");
		sorted->sis_item = item;
#define	LESS_THAN(a, b)							\
		(((a)->sis_item->si_component <				\
		  (b)->sis_item->si_component) ||			\
		 ((a)->sis_item->si_order <				\
		  (b)->sis_item->si_order))

		if (first == NULL || LESS_THAN(sorted, first)) {
			if (first != NULL)
				first->sis_prev = sorted;
			sorted->sis_prev = NULL;
			sorted->sis_next = first;
			first = sorted;
		} else {
			for (ip = first; ip != NULL; ip = ip->sis_next) {
				if (LESS_THAN(sorted, ip)) {
					sorted->sis_next = ip;
					if (ip->sis_prev != NULL)
						ip->sis_prev->sis_next = sorted;
					sorted->sis_prev = ip->sis_prev;
					ip->sis_prev = sorted;
					goto next;
				}
			}
			for (ip = first; ip->sis_next != NULL;
			     ip = ip->sis_next) {
				continue;
			}
			ip->sis_next = sorted;
			sorted->sis_prev = ip;
			sorted->sis_next = NULL;
		}
#undef LESS_THAN
next:		continue;
	}
	for (ip = first; ip != NULL; ip = ip->sis_next) {
		item = ip->sis_item;
		item->si_function(item->si_arg);
	}
}

void
startup_main(void)
{
	struct thread *thread;
	bool bootstrap;
	int error;

	spinlock_lock(&startup_spinlock);
	bootstrap = !startup_booting;
	startup_booting = true;

	if (bootstrap) {
		error = task_create(&main_task, NULL, "main", TASK_KERNEL);
		if (error != 0)
			panic("%s: task_create failed: %m", __func__, error);
	}

	error = thread_create(&thread, main_task, "thread", THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);

	spinlock_unlock(&startup_spinlock);

	/*
	 * XXX
	 * Switch to running our thread.
	 *
	 * I don't want to hear
	 * I don't want to know
	 * Please don't say, "forgive me"
	 * I've seen it all before
	 * And I
	 * Can't take it anymore
	 */

	if (bootstrap)
		startup_boot();

	for (;;) {
	}
}

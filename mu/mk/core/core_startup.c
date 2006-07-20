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

static void startup_boot_thread(void *);
static void startup_main_thread(void *);
STARTUP_ITEM(main, STARTUP_MAIN, STARTUP_FIRST, startup_main_thread, NULL);

static bool startup_booting = false;
static struct spinlock startup_spinlock = SPINLOCK_INIT("startup");
static struct task *main_task;

void
startup_main(void)
{
	struct thread *td;
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

	error = thread_create(&td, main_task, "scheduler",
			      THREAD_DEFAULT | THREAD_PINNED);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);

	spinlock_unlock(&startup_spinlock);

	if (bootstrap)
		thread_set_upcall(td, startup_boot_thread, td);
	else
		thread_set_upcall(td, startup_main_thread, td);
	PCPU_SET(idletd, td);
	thread_block();			/* Bam! */
}

static void
startup_boot_thread(void *arg)
{
	struct thread *td;
	struct startup_item **itemp, *item;
	struct startup_item_sorted *first, *sorted, *ip;
	struct pool startup_item_pool;
	int error;

	td = arg;
	ASSERT(td == current_thread(), "booting with wrong thread");

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
		/* XXX lord of dirty hacks.  */
		if (item->si_component == STARTUP_MAIN) {
			ASSERT(td != NULL, "must have a thread.");
			item->si_function(td);
		} else {
			item->si_function(item->si_arg);
		}
	}
}

static void
startup_main_thread(void *arg)
{
	static uint64_t threadcnt;
	uint64_t last;
	struct thread *td;

	last = 0;
	td = arg;
	ASSERT(td == current_thread(), "consistency is all I ask");
	atomic_increment_64(&threadcnt);
	for (;;) {
		unsigned long now;

		now = atomic_load_64(&threadcnt);
		if (now - last > 1) {
			kcprintf("cpu%u: threadcnt went from %lu to %lu\n",
				 mp_whoami(), last, now);
		}
		last = now;
	}
}

#include <core/types.h>
#include <core/ipc.h>
#include <core/pool.h>
#include <core/scheduler.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <io/device/console/console.h>

struct startup_item_sorted {
	struct startup_item *sis_item;
	TAILQ_ENTRY(struct startup_item_sorted) sis_link;
};

SET(startup_items, struct startup_item);

static void startup_bootstrap(void);
static void startup_boot_thread(void *);
static void startup_idle_thread(void *);

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

	if (bootstrap)
		startup_bootstrap();

	error = thread_create(&td, main_task, "idle thread",
			      THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);
	scheduler_cpu_idle(td);

	spinlock_unlock(&startup_spinlock);

	if (bootstrap)
		thread_set_upcall(td, startup_boot_thread, NULL);
	else
		thread_set_upcall(td, startup_idle_thread, NULL);
	scheduler_schedule();
}

static void
startup_bootstrap(void)
{
	int error;

	/*
	 * Initialize IPC functionality.
	 */
	ipc_init();

	/*
	 * Create our first task.
	 */
	error = task_create(&main_task, NULL, "main", TASK_KERNEL);
	if (error != 0)
		panic("%s: task_create failed: %m", __func__, error);
}

static void
startup_boot_thread(void *arg)
{
	struct startup_item **itemp, *item;
	struct startup_item_sorted *sorted, *ip;
	TAILQ_HEAD(, struct startup_item_sorted) sortedq;
	struct pool startup_item_pool;
	int error;

	kcprintf("The system is coming up.\n");

	error = pool_create(&startup_item_pool, "startup item", sizeof *sorted,
			    POOL_VIRTUAL);
	TAILQ_INIT(&sortedq);
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

		if (TAILQ_EMPTY(&sortedq) ||
		    LESS_THAN(sorted, TAILQ_FIRST(&sortedq))) {
			TAILQ_INSERT_HEAD(&sortedq, sorted, sis_link);
		} else {
			TAILQ_FOREACH(ip, &sortedq, sis_link) {
				if (LESS_THAN(sorted, ip)) {
					TAILQ_INSERT_BEFORE(ip, sorted, sis_link);
					goto next;
				}
			}
			TAILQ_INSERT_TAIL(&sortedq, sorted, sis_link);
		}
#undef LESS_THAN
next:		continue;
	}
	TAILQ_FOREACH(ip, &sortedq, sis_link) {
		item = ip->sis_item;
		item->si_function(item->si_arg);
	}
}

static void
startup_idle_thread(void *arg)
{
	ipc_process();
}
STARTUP_ITEM(main, STARTUP_MAIN, STARTUP_FIRST, startup_idle_thread, NULL);

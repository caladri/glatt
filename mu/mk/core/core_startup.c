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
#include <vm/vm.h>

SET(startup_items, struct startup_item);

static void startup_bootstrap(void);
static void startup_boot_thread(void *);
static void startup_main_thread(void *);

static bool startup_booting = false;
static struct spinlock startup_spinlock = SPINLOCK_INIT("startup");
static struct task *main_task;

void
startup_init(void)
{
#ifdef	UNIPROCESSOR
	kcprintf("STARTUP: Uniprocessor kernel, only the boot CPU will be used.\n");
#endif

	/*
	 * Turn on the virtual memory subsystem.
	 */
	vm_init();

	/*
	 * Set up scheduler subsystem.
	 */
	scheduler_init();

	/*
	 * Initialize IPC functionality.
	 */
	ipc_init();
}

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

	error = thread_create(&td, main_task, "main thread",
			      THREAD_DEFAULT);
	if (error != 0)
		panic("%s: thread_create failed: %m", __func__, error);
	if (bootstrap)
		thread_set_upcall(td, cpu_startup_thread,
				  (void *)(uintptr_t)startup_boot_thread);
	else
		thread_set_upcall(td, cpu_startup_thread,
				  (void *)(uintptr_t)startup_main_thread);
	scheduler_cpu_pin(td);
	scheduler_thread_runnable(td);
	spinlock_unlock(&startup_spinlock);

	scheduler_schedule();
}

static void
startup_bootstrap(void)
{
	int error;

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
	struct startup_item **itemp, *item, *ip;
	static TAILQ_HEAD(, struct startup_item) sorted_items;

	TAILQ_INIT(&sorted_items);

#ifdef	VERBOSE
	kcprintf("STARTUP: The system is coming up.\n");
#endif

	spinlock_lock(&startup_spinlock);
	for (itemp = SET_BEGIN(startup_items); itemp < SET_END(startup_items);
	     itemp++) {
		item = *itemp;
		if (item->si_order == STARTUP_CPU)
			continue;
#define	LESS_THAN(a, b)							\
		(((a)->si_component < (b)->si_component) ||		\
		 (((a)->si_component == (b)->si_component) &&		\
		  ((a)->si_order < (b)->si_order)))
		if (TAILQ_EMPTY(&sorted_items) ||
		    LESS_THAN(item, TAILQ_FIRST(&sorted_items))) {
			TAILQ_INSERT_HEAD(&sorted_items, item, si_link);
		} else {
			TAILQ_FOREACH(ip, &sorted_items, si_link) {
				if (LESS_THAN(item, ip)) {
					TAILQ_INSERT_BEFORE(ip, item, si_link);
					goto next;
				}
			}
			TAILQ_INSERT_TAIL(&sorted_items, item, si_link);
		}
#undef LESS_THAN
next:		continue;
	}

	/*
	 * Don't let other threads run until we're done starting up.
	 */
	TAILQ_FOREACH(item, &sorted_items, si_link)
		item->si_function(item->si_arg);
	ASSERT(false, "Must not be reached.");
}

static void
startup_main_thread(void *arg)
{
	struct startup_item **itemp, *item, *ip;
	TAILQ_HEAD(, struct startup_item) sorted_items;
	struct spinlock *lock;

	lock = arg;

	TAILQ_INIT(&sorted_items);

	/*
	 * The boot thread will come here and need to unlock the startup
	 * spinlock.
	 */
	if (lock != NULL)
		spinlock_unlock(lock);

#ifdef	VERBOSE
	kcprintf("STARTUP: cpu%u starting main thread.\n", mp_whoami());
#endif

	spinlock_lock(&startup_spinlock);
	for (itemp = SET_BEGIN(startup_items); itemp < SET_END(startup_items);
	     itemp++) {
		item = *itemp;
		if (item->si_order != STARTUP_CPU)
			continue;
#define	LESS_THAN(a, b)							\
		(((a)->si_component < (b)->si_component) ||		\
		 (((a)->si_component == (b)->si_component) &&		\
		  ((a)->si_order < (b)->si_order)))
		if (TAILQ_EMPTY(&sorted_items) ||
		    LESS_THAN(item, TAILQ_FIRST(&sorted_items))) {
			TAILQ_INSERT_HEAD(&sorted_items, item, si_link);
		} else {
			TAILQ_FOREACH(ip, &sorted_items, si_link) {
				if (LESS_THAN(item, ip)) {
					TAILQ_INSERT_BEFORE(ip, item, si_link);
					goto next;
				}
			}
			TAILQ_INSERT_TAIL(&sorted_items, item, si_link);
		}
#undef LESS_THAN
next:		continue;
	}

	/* Initialize per-CPU structures.  */
	TAILQ_FOREACH(item, &sorted_items, si_link)
		item->si_function(item->si_arg);
	spinlock_unlock(&startup_spinlock);

	/*
	 * Threaded initialialization complete, the scheduler may run other
	 * threads now.
	 */
	scheduler_cpu_switchable();

	/*
	 * Process IPC ad absurdum.
	 */
	ipc_process();
}
STARTUP_ITEM(main, STARTUP_MAIN, STARTUP_FIRST, startup_main_thread, &startup_spinlock);

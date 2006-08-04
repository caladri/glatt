#include <core/types.h>
#include <core/scheduler.h>
#include <core/thread.h>

static struct thread *scheduler_pick(void);
static void scheduler_switch(struct thread *);
static void scheduler_yield(void);

void
scheduler_schedule(void)
{
	struct thread *td;

	/*
	 * Find a thread to run.  If there's nothing in the runqueue, check to
	 * see if we're the idle thread.  If we are the idle thread, yield the
	 * CPU until we get an interrupt.  Otherwise, without a thread to run,
	 * switch to the idle thread.
	 */
	td = scheduler_pick();
	if (td == NULL) {
		if (PCPU_GET(idletd) == current_thread()) {
			scheduler_yield();
			return;
		}
		td = PCPU_GET(idletd);
	}
	scheduler_switch(td);
}

static struct thread *
scheduler_pick(void)
{
	/*
	 * XXX check runqueue and per-CPU runqueue?
	 */
	return (NULL);
}

static void
scheduler_switch(struct thread *td)
{
	/*
	 * XXX update statistics, and so on?
	 */
	thread_switch(NULL, td);
}

static void
scheduler_yield(void)
{
	/* XXX cpu_yield(); ? */
}

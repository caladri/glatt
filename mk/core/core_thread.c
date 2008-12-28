#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/spinlock.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>

static struct pool thread_pool;
static struct spinlock thread_lock;

static void thread_error(void *);

void
thread_init(void)
{
	int error;

	error = pool_create(&thread_pool, "THREAD", sizeof (struct thread),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	spinlock_init(&thread_lock, "THREAD", SPINLOCK_FLAG_DEFAULT);
}

int
thread_create(struct thread **tdp, struct task *task, const char *name,
	      unsigned flags)
{
	struct thread *td;
	int error;

	ASSERT(task != NULL, "thread needs a task");

	td = pool_allocate(&thread_pool);
	if (td == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(td->td_name, name, sizeof td->td_name);
	td->td_task = task;
	STAILQ_INSERT_TAIL(&task->t_threads, td, td_link);
	td->td_flags = flags;

	scheduler_thread_setup(td);

	error = cpu_thread_setup(td);
	if (error != 0) {
		panic("%s: need to destroy thread, cpu_thread_setup failed: %m",
		      __func__, error);
	}
	*tdp = td;
	return (0);
}

void
thread_set_upcall(struct thread *td, void (*function)(void *), void *arg)
{
	cpu_thread_set_upcall(td, function, arg);
}

void
thread_switch(struct thread *otd, struct thread *td)
{
	if (otd == NULL)
		otd = current_thread();
	ASSERT(otd != td, "cannot switch from a thread to itself.");
	if (otd != NULL) {
		if (cpu_context_save(otd)) {
			thread_set_upcall(otd, thread_error, otd);
			spinlock_unlock(&thread_lock);
			/*
			 * We've been restored by something, return.
			 */
			return;
		}
	}
	spinlock_lock(&thread_lock);
	cpu_context_restore(td);
}

void
thread_trampoline(struct thread *td, void (*function)(void *), void *arg)
{
	spinlock_unlock(&thread_lock);
	ASSERT(td == current_thread(), "Thread must be current thread.");
	function(arg);
	panic("%s: function returned!", __func__);
}

static void
thread_error(void *arg)
{
	struct thread *td = arg;

	panic("%s: context re-used for thread %p (%s)", __func__, td,
	      td->td_name);
}

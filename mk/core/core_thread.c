#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>

static struct pool thread_pool;

static void thread_error(struct thread *, void *);

void
thread_init(void)
{
	int error;

	error = pool_create(&thread_pool, "THREAD", sizeof (struct thread),
			    POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
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
thread_exit(void)
{
	scheduler_thread_exiting();
	scheduler_schedule(NULL, NULL);
	NOTREACHED();
}

void
thread_free(struct thread *td)
{
	struct task *task;

	task = td->td_task;

	STAILQ_REMOVE(&task->t_threads, td, struct thread, td_link);

	scheduler_thread_free(td);

	cpu_thread_free(td);

	pool_free(td);

	if (STAILQ_EMPTY(&task->t_threads))
		task_free(task);
}

void
thread_set_upcall(struct thread *td, void (*function)(struct thread *, void *), void *arg)
{
	cpu_thread_set_upcall(td, function, arg);
}

void
thread_switch(struct thread *otd, struct thread *td)
{
	ASSERT(critical_section(), "cannot switch outside a critical section.");
	if (otd == NULL)
		otd = current_thread();
	ASSERT(otd != td, "cannot switch from a thread to itself.");
	if (otd != NULL) {
		if (cpu_context_save(otd)) {
			otd = current_thread();
			thread_set_upcall(otd, thread_error, otd);
			/*
			 * We've been restored by something, return.
			 */
			return;
		}
	}
	cpu_context_restore(td);
}

void
thread_trampoline(struct thread *td, void (*function)(struct thread *, void *), void *arg)
{
	scheduler_activate(td);
	ASSERT(td == current_thread(), "Thread must be current thread.");
	ASSERT(function != NULL, "Function must not be NULL.");
	function(td, arg);
	panic("%s: function returned!", __func__);
}

static void
thread_error(struct thread *td, void *arg)
{
	ASSERT(td == arg, ("thread and argument mismatch"));
	panic("%s: context re-used for thread %p (%s)", __func__, td,
	      td->td_name);
}

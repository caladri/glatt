#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/spinlock.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <vm/page.h>

/*
 * Determines which allocator we use.  We can switch to vm_alloc if the thread
 * structure gets bigger than a PAGE.
 */
COMPILE_TIME_ASSERT(sizeof (struct thread) <= PAGE_SIZE);

static struct pool thread_pool = POOL_INIT("THREAD", struct thread, POOL_VIRTUAL);

static struct spinlock thread_lock = SPINLOCK_INIT("THREAD");

static void thread_error(void *);

int
thread_create(struct thread **tdp, struct task *parent, const char *name,
	      uint32_t flags)
{
	struct thread *td;
	int error;

	ASSERT(parent != NULL, "thread needs a task");

	td = pool_allocate(&thread_pool);
	if (td == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(td->td_name, name, sizeof td->td_name);
	td->td_parent = parent;
	STAILQ_INSERT_TAIL(&parent->t_threads, td, td_link);
	td->td_flags = flags;

#ifndef	NO_MORDER
	morder_thread_setup(td);
#endif
	scheduler_thread_setup(td);

	error = cpu_thread_setup(td);
	if (error != 0) {
		panic("%s: need to destroy thread, cpu_thread_setup failed: %m",
		      __func__, error);
#if 0
		return (error);
#endif
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

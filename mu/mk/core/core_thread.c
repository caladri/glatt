#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <vm/page.h>

#include <io/device/console/console.h>

/*
 * Determines which allocator we use.  We can switch to vm_alloc if the task
 * structure gets bigger than a PAGE.
 */
COMPILE_TIME_ASSERT(sizeof (struct thread) <= PAGE_SIZE);

static struct pool thread_pool = POOL_INIT("THREAD", struct thread, POOL_VIRTUAL);

void
thread_block(void)
{
	struct thread *td;

	td = current_thread();
	if (td != NULL) {
		ASSERT((td->td_flags & THREAD_BLOCKED) == 0,
		       "can't block a blocked (but running!) thread.");
		td->td_flags |= THREAD_BLOCKED;
	}
	thread_switch(td, PCPU_GET(idletd));
}

int
thread_create(struct thread **tdp, struct task *parent, const char *name,
	      uint32_t flags)
{
	struct thread *td;
	int error;

	td = pool_allocate(&thread_pool);
	if (td == NULL)
		return (ERROR_EXHAUSTED);
	strlcpy(td->td_name, name, sizeof td->td_name);
	td->td_parent = parent;
	td->td_next = parent->t_threads;
	parent->t_threads = td;
	td->td_flags = flags;
	td->td_oncpu = CPU_ID_INVALID;
	/*
	 * CPU thread setup takes care of:
	 * 	frame
	 * 	context
	 * 	kstack
	 */
	error = cpu_thread_setup(td);
	if (error != 0) {
		panic("%s: need to destroy thread, cpu_thread_setup failed: %m",
		      __func__, error);
		return (error);
	}
	*tdp = td;
	return (0);
}

void
thread_set_upcall(struct thread *td, void (*function)(void *), void *arg)
{
	kcprintf("cpu%u: Set upcall on %p\n", mp_whoami(), (void *)td);
	cpu_thread_set_upcall(td, function, arg);
}

void
thread_switch(struct thread *otd, struct thread *td)
{
	ASSERT((td->td_flags & THREAD_BLOCKED) == 0,
	       "can't switch to a blocked thread, use wakeup.");
	if (otd == NULL)
		otd = current_thread();
	ASSERT(otd != td, "cannot switch from a thread to itself.");
	if (otd != NULL) {
		otd->td_flags &= ~THREAD_RUNNING;
		otd->td_oncpu = CPU_ID_INVALID;
		if (cpu_context_save(otd)) {
			ASSERT(otd->td_oncpu == mp_whoami(),
			       "need valid oncpu field.");
			ASSERT((otd->td_flags & THREAD_RUNNING) != 0,
			       "thread must be marked running.");
			/*
			 * We've been restored by something, return.
			 */
			return;
		}
	}
	kcprintf("cpu%u: Switching from %p to %p\n", mp_whoami(), (void *)otd, (void *)td);
	td->td_flags |= THREAD_RUNNING;
	td->td_oncpu = mp_whoami();
	cpu_context_restore(td);
}

void
thread_trampoline(struct thread *td, void (*function)(void *), void *arg)
{
	kcprintf("cpu%u: Trampolined into %p\n", mp_whoami(), (void *)td);
	ASSERT(td->td_oncpu == mp_whoami(), "need valid oncpu field.");
	ASSERT((td->td_flags & THREAD_RUNNING) != 0,
	       "thread must be marked running.");
	function(arg);
	panic("%s: function returned!", __func__);
}

void
thread_wakeup(struct thread *td)
{
	ASSERT((td->td_flags & THREAD_BLOCKED) != 0,
	       "can only wake up blocked threads.");
	ASSERT(current_thread() == PCPU_GET(idletd),
	       "must wake up from idle thread.");
	td->td_flags &= ~THREAD_BLOCKED;
	thread_switch(NULL, td);
}

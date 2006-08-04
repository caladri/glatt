#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
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
	td->td_next = parent->t_threads;
	parent->t_threads = td;
	td->td_flags = flags;
	if ((td->td_flags & THREAD_PINNED) != 0)
		td->td_oncpu = mp_whoami();
	else
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
	cpu_thread_set_upcall(td, function, arg);
}

void
thread_switch(struct thread *otd, struct thread *td)
{
	if (otd == NULL)
		otd = current_thread();
	ASSERT(otd != td, "cannot switch from a thread to itself.");
	if (otd != NULL) {
		otd->td_flags &= ~THREAD_RUNNING;
		if ((otd->td_flags & THREAD_PINNED) == 0)
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
	if (td->td_oncpu != CPU_ID_INVALID &&
	    (td->td_oncpu != mp_whoami() ||
	     (td->td_flags & THREAD_PINNED) == 0)) {
		panic("%s: thread still on a CPU but not pinned to me.",
		      __func__);
	}
	td->td_flags |= THREAD_RUNNING;
	td->td_oncpu = mp_whoami();
	cpu_context_restore(td);
}

void
thread_trampoline(struct thread *td, void (*function)(void *), void *arg)
{
	ASSERT(td->td_oncpu == mp_whoami(), "need valid oncpu field.");
	ASSERT((td->td_flags & THREAD_RUNNING) != 0,
	       "thread must be marked running.");
	function(arg);
	panic("%s: function returned!", __func__);
}

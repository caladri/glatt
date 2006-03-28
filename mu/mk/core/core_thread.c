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
	kcprintf("Set upcall on %p\n", (void *)td);
	cpu_thread_set_upcall(td, function, arg);
}

void
thread_switch(struct thread *otd, struct thread *td)
{
	if (otd == NULL)
		otd = current_thread();
	if (otd != NULL) {
		if (cpu_context_save(otd)) {
			/*
			 * We've been restored by something, return.
			 */
			return;
		}
	}
	kcprintf("Switching from %p to %p\n", (void *)otd, (void *)td);
	cpu_context_restore(td);
}

void
thread_trampoline(struct thread *td, void (*function)(void *), void *arg)
{
	kcprintf("Trampolined into %p\n", (void *)td);
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

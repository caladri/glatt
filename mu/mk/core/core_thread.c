#include <core/types.h>
#include <core/error.h>
#include <core/thread.h>
#include <vm/page.h>

/*
 * Determines which allocator we use.  We can switch to vm_alloc if the task
 * structure gets bigger than a PAGE.
 */
COMPILE_TIME_ASSERT(sizeof (struct thread) <= PAGE_SIZE);

int
thread_create(struct thread **tdp, struct task *parent, const char *name,
	      uint32_t flags)
{
	return (ERROR_NOT_IMPLEMENTED);
}

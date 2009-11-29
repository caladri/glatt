#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <core/ttk.h>

void
ttk_idle(void)
{
	ASSERT(!critical_section(),
	       "Must not be idling in a critical section.");

	/*
	 * XXX
	 * Use a CPU wait instruction if there's nothing to run.
	 * Does that need done in the scheduler?
	 */
	scheduler_schedule(NULL, NULL);
}

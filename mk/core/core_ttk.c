#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <core/ttk.h>
#include <cpu/startup.h>

void
ttk_idle(void)
{
	ASSERT(!critical_section(),
	       "Must not be idling in a critical section.");

	/*
	 * If the scheduler doesn't know of anything we can run,
	 * idle this CPU.  This has a race, but if the race is
	 * lost the delay is only about a tick.
	 */
	while (scheduler_idle())
		cpu_wait();

	scheduler_schedule(NULL, NULL);
}

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
	struct thread *td;

	ASSERT(!critical_section(),
	       "Must not be idling in a critical section.");

	td = current_thread();

	/* XXX cpu_ttk_wait();  */
#if __mips__
	asm volatile ("wait");
#endif

	scheduler_schedule(NULL, NULL);
}

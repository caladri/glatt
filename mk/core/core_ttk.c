#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <core/ttk.h>

void
ttk_idle(void)
{
	struct thread *td;

	td = current_thread();

	/* XXX cpu_ttk_wait();  */
#if __mips__
	__asm__ __volatile__ ("wait");
#endif

	scheduler_schedule(NULL, NULL);
}

#include <core/types.h>
#include <core/error.h>
#include <core/syscall.h>
#include <core/thread.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>
#include <io/console/console.h>

int
syscall(unsigned number, register_t *cnt, register_t *params)
{
	struct thread *td;

	td = current_thread();

	switch (number) {
	case SYSCALL_EXIT:
		if (*cnt != 0)
			return (ERROR_INVALID);
		thread_exit();
		/* NOTREACHED */
	case SYSCALL_CONSOLE_WRITE:
		if (*cnt != 1)
			return (ERROR_INVALID);
		/*
		 * XXX
		 * copyin of some sort.
		 */
		kcprintf("%s: %s\n", td->td_name, (const char *)(uintptr_t)params[0]);
		return (0);
	default:
		return (ERROR_INVALID);
	}
}

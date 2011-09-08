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
	int error;
	char ch;

	td = current_thread();

	switch (number) {
	case SYSCALL_EXIT:
		if (*cnt != 0)
			return (ERROR_INVALID);
		thread_exit();
		/* NOTREACHED */
	case SYSCALL_CONSOLE_PUTC:
		if (*cnt != 1)
			return (ERROR_INVALID);
		kcputc(params[0]);
		return (0);
	case SYSCALL_CONSOLE_GETC:
		if (*cnt != 0)
			return (ERROR_INVALID);
		error = kcgetc(&ch);
		if (error != 0)
			return (error);
		*cnt = 1;
		params[0] = ch;
		return (0);
	default:
		return (ERROR_INVALID);
	}
}

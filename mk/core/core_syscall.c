#include <core/types.h>
#include <core/error.h>
#include <core/syscall.h>
#include <core/thread.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>

int
syscall(unsigned number, register_t *cnt, register_t *params)
{
	switch (number) {
	case SYSCALL_EXIT:
		if (*cnt != 0)
			return (ERROR_INVALID);
		thread_exit();
		/* NOTREACHED */
	default:
		return (ERROR_INVALID);
	}
}

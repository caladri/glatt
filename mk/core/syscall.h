#ifndef	_CORE_SYSCALL_H_
#define	_CORE_SYSCALL_H_

#include <cpu/register.h>

#define	SYSCALL_EXIT		(0)
#define	SYSCALL_CONSOLE_WRITE	(1)

int syscall(unsigned, register_t *, register_t *);

#endif /* !_CORE_SYSCALL_H_ */

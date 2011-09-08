#ifndef	_CORE_SYSCALL_H_
#define	_CORE_SYSCALL_H_

#include <cpu/register.h>

#define	SYSCALL_EXIT		(0)
#define	SYSCALL_CONSOLE_PUTC	(1)
#define	SYSCALL_CONSOLE_GETC	(2)

#ifndef	ASSEMBLER
int syscall(unsigned, register_t *, register_t *);
#endif

#endif /* !_CORE_SYSCALL_H_ */

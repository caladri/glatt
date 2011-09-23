#ifndef	_CORE_SYSCALL_H_
#define	_CORE_SYSCALL_H_

#include <cpu/register.h>

#define	SYSCALL_EXIT			(0)
#define	SYSCALL_CONSOLE_PUTC		(1)
#define	SYSCALL_IPC_PORT_ALLOCATE	(2)
#define	SYSCALL_IPC_PORT_SEND		(3)
#define	SYSCALL_IPC_PORT_WAIT		(4)
#define	SYSCALL_VM_PAGE_GET		(5)
#define	SYSCALL_VM_PAGE_FREE		(6)

#ifdef MK
#ifndef	ASSEMBLER
int syscall(unsigned, register_t *, register_t *);
#endif
#endif

#endif /* !_CORE_SYSCALL_H_ */

#ifndef	_CORE_SYSCALL_H_
#define	_CORE_SYSCALL_H_

#include <cpu/register.h>

#define	SYSCALL_BASE			(0x00)
#define	SYSCALL_EXIT			(SYSCALL_BASE + 0x00)

#define	SYSCALL_CONSOLE_BASE		(0x10)
#define	SYSCALL_CONSOLE_PUTC		(SYSCALL_CONSOLE_BASE + 0x00)

#define	SYSCALL_IPC_BASE		(0x20)
#define	SYSCALL_IPC_PORT_ALLOCATE	(SYSCALL_IPC_BASE + 0x00)
#define	SYSCALL_IPC_PORT_SEND		(SYSCALL_IPC_BASE + 0x01)
#define	SYSCALL_IPC_PORT_WAIT		(SYSCALL_IPC_BASE + 0x02)
#define	SYSCALL_IPC_PORT_RECEIVE	(SYSCALL_IPC_BASE + 0x03)

#define	SYSCALL_VM_BASE			(0x30)
#define	SYSCALL_VM_PAGE_GET		(SYSCALL_VM_BASE + 0x00)
#define	SYSCALL_VM_PAGE_FREE		(SYSCALL_VM_BASE + 0x01)

#ifdef MK
#ifndef	ASSEMBLER
int syscall(unsigned, register_t *, register_t *);
#endif
#endif

#endif /* !_CORE_SYSCALL_H_ */

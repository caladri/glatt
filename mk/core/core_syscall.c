#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <core/syscall.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>
#include <io/console/console.h>
#ifdef IPC
#include <ipc/ipc.h>
#include <ipc/port.h>
#endif
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_page.h>

int
syscall(unsigned number, register_t *cnt, register_t *params)
{
#ifdef IPC
	struct ipc_header ipch;
#endif
	struct thread *td;
#ifdef IPC
	ipc_port_t port;
#endif
	vaddr_t vaddr;
	int error;
#ifdef IPC
	void *p;
#endif

	td = current_thread();

	switch (number) {
	case SYSCALL_EXIT:
		if (*cnt != 0)
			return (ERROR_ARG_COUNT);
		thread_exit();
		/* NOTREACHED */
	case SYSCALL_CONSOLE_PUTC:
		if (*cnt != 1)
			return (ERROR_ARG_COUNT);
		kcputc(params[0]);
		*cnt = 0;
		return (0);
	case SYSCALL_CONSOLE_PUTS:
		if (*cnt != 2)
			return (ERROR_ARG_COUNT);
		kcputsn((void *)(uintptr_t)params[0], params[1]); /* XXX copyinstr.  */
		*cnt = 0;
		return (0);
	case SYSCALL_IPC_PORT_ALLOCATE:
#ifdef IPC
		if (*cnt != 1)
			return (ERROR_ARG_COUNT);
		error = ipc_port_allocate(td->td_task, &port, (ipc_port_flags_t)params[0]);
		if (error != 0)
			return (error);
		*cnt = 1;
		params[0] = port;
		return (0);
#else
		return (ERROR_NOT_AVAILABLE);
#endif
	case SYSCALL_IPC_PORT_SEND:
#ifdef IPC
		if (*cnt != 2)
			return (ERROR_ARG_COUNT);
		/*
		 * XXX
		 * Would be nice to avoid so many copies of the header.
		 */
		memcpy(&ipch, (void *)(uintptr_t)params[0], sizeof ipch); /* XXX copyin.  */
		error = ipc_port_send(&ipch, (void *)(uintptr_t)params[1]);
		if (error != 0)
			return (error);
		*cnt = 0;
		return (0);
#else
		return (ERROR_NOT_AVAILABLE);
#endif
	case SYSCALL_IPC_PORT_WAIT:
#ifdef IPC
		if (*cnt != 1)
			return (ERROR_ARG_COUNT);
		error = ipc_port_wait((ipc_port_t)params[0]);
		if (error != 0)
			return (error);
		*cnt = 0;
		return (0);
#else
		return (ERROR_NOT_AVAILABLE);
#endif
	case SYSCALL_IPC_PORT_RECEIVE:
#ifdef IPC
		if (*cnt != 2)
			return (ERROR_ARG_COUNT);
		error = ipc_port_receive((ipc_port_t)params[0], &ipch, &p);
		if (error != 0)
			return (error);
		memcpy((void *)(uintptr_t)params[1], &ipch, sizeof ipch); /* XXX copyout */
		*cnt = 1;
		params[0] = (register_t)(intptr_t)p;
		return (0);
#else
		return (ERROR_NOT_AVAILABLE);
#endif
	case SYSCALL_VM_PAGE_GET:
		if (*cnt != 0)
			return (ERROR_ARG_COUNT);
		error = vm_alloc_page(td->td_task->t_vm, &vaddr);
		if (error != 0)
			return (error);
		*cnt = 1;
		params[0] = vaddr;
		return (0);
	case SYSCALL_VM_PAGE_FREE:
		if (*cnt != 1)
			return (ERROR_ARG_COUNT);
		error = vm_free_page(td->td_task->t_vm, (vaddr_t)params[0]);
		if (error != 0)
			return (error);
		*cnt = 0;
		return (0);

		return (ERROR_NOT_IMPLEMENTED);
	default:
		return (ERROR_INVALID);
	}
}

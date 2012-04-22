#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <core/syscall.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>
#include <core/console.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_page.h>

typedef	int (syscall_handler_t)(register_t *);

struct syscall_vector {
	unsigned sv_inputs;
	unsigned sv_outputs;
	syscall_handler_t *sv_handler;
};

static syscall_handler_t syscall_exit;

static syscall_handler_t syscall_console_putc,
			 syscall_console_puts,
			 syscall_console_getc;

static syscall_handler_t syscall_ipc_port_allocate,
			 syscall_ipc_port_send,
			 syscall_ipc_port_wait,
			 syscall_ipc_port_receive;

static syscall_handler_t syscall_vm_page_get,
			 syscall_vm_page_free;

static struct syscall_vector syscall_vector[SYSCALL_LAST + 1] = {
	[SYSCALL_EXIT] =		{ 0, 0, syscall_exit },

	[SYSCALL_CONSOLE_PUTC] =	{ 1, 0, syscall_console_putc },
	[SYSCALL_CONSOLE_PUTS] =	{ 2, 0, syscall_console_puts },
	[SYSCALL_CONSOLE_GETC] =	{ 0, 1, syscall_console_getc },

	[SYSCALL_IPC_PORT_ALLOCATE] =	{ 1, 1, syscall_ipc_port_allocate },
	[SYSCALL_IPC_PORT_SEND] =	{ 2, 0, syscall_ipc_port_send },
	[SYSCALL_IPC_PORT_WAIT] =	{ 1, 0, syscall_ipc_port_wait },
	[SYSCALL_IPC_PORT_RECEIVE] =	{ 2, 1, syscall_ipc_port_receive },

	[SYSCALL_VM_PAGE_GET] =		{ 0, 1, syscall_vm_page_get },
	[SYSCALL_VM_PAGE_FREE] =	{ 1, 0, syscall_vm_page_free },
};

int
syscall(unsigned number, register_t *cnt, register_t *params)
{
	struct syscall_vector *sv;
	int error;

	if (number > SYSCALL_LAST)
		return (ERROR_NOT_AVAILABLE);

	sv = &syscall_vector[number];
	if (sv->sv_handler == NULL)
		return (ERROR_NOT_AVAILABLE);

	if (*cnt != sv->sv_inputs)
		return (ERROR_ARG_COUNT);

	error = sv->sv_handler(params);
	if (error != 0)
		return (error);

	*cnt = sv->sv_outputs;
	return (0);
}

static int
syscall_exit(register_t *params)
{
	(void)params;
	thread_exit();
	/* NOTREACHED */
}

static int
syscall_console_putc(register_t *params)
{
	kcputc(params[0]);
	return (0);
}

static int
syscall_console_puts(register_t *params)
{
	kcputsn((void *)(uintptr_t)params[0], params[1]); /* XXX copyinstr.  */
	return (0);
}

static int
syscall_console_getc(register_t *params)
{
	int error;
	char ch;

	error = kcgetc_wait(&ch);
	if (error != 0)
		return (error);

	params[0] = ch;
	return (0);
}

static int
syscall_ipc_port_allocate(register_t *params)
{
	ipc_port_t port;
	int error;

	error = ipc_port_allocate(&port, (ipc_port_flags_t)params[0]);
	if (error != 0)
		return (error);

	params[0] = port;
	return (0);
}

static int
syscall_ipc_port_send(register_t *params)
{
	struct ipc_header ipch;
	int error;

	/*
	 * XXX
	 * Would be nice to avoid so many copies of the header.
	 */
	memcpy(&ipch, (void *)(uintptr_t)params[0], sizeof ipch); /* XXX copyin.  */
	error = ipc_port_send(&ipch, (void *)(uintptr_t)params[1]);
	if (error != 0)
		return (error);
	return (0);
}

static int
syscall_ipc_port_wait(register_t *params)
{
	int error;

	error = ipc_port_wait((ipc_port_t)params[0]);
	if (error != 0)
		return (error);
	return (0);
}

static int
syscall_ipc_port_receive(register_t *params)
{
	struct ipc_header ipch;
	int error;
	void *p;

	error = ipc_port_receive((ipc_port_t)params[0], &ipch, &p);
	if (error != 0)
		return (error);

	params[0] = (register_t)(intptr_t)p;
	memcpy((void *)(uintptr_t)params[1], &ipch, sizeof ipch); /* XXX copyout */
	return (0);
}

static int
syscall_vm_page_get(register_t *params)
{
	struct thread *td;
	vaddr_t vaddr;
	int error;

	td = current_thread();

	error = vm_alloc_page(td->td_task->t_vm, &vaddr);
	if (error != 0)
		return (error);
	params[0] = vaddr;
	return (0);
}

static int
syscall_vm_page_free(register_t *params)
{
	struct thread *td;
	int error;

	td = current_thread();

	error = vm_free_page(td->td_task->t_vm, (vaddr_t)params[0]);
	if (error != 0)
		return (error);

	return (0);
}

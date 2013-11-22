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
#include <vm/vm_index.h>
#include <vm/vm_page.h>

typedef	int (syscall_handler_t)(register_t *);

struct syscall_vector {
	unsigned sv_inputs;
	unsigned sv_outputs;
	syscall_handler_t *sv_handler;
};

static syscall_handler_t syscall_thread_exit,
			 syscall_thread_create;

static syscall_handler_t syscall_console_putc,
			 syscall_console_puts,
			 syscall_console_getc;

static syscall_handler_t syscall_ipc_port_allocate,
			 syscall_ipc_port_send,
			 syscall_ipc_port_wait,
			 syscall_ipc_port_receive,
			 syscall_ipc_task_port;

static syscall_handler_t syscall_vm_page_get,
			 syscall_vm_page_free,
			 syscall_vm_alloc,
			 syscall_vm_free,
			 syscall_vm_alloc_range;

static struct syscall_vector syscall_vector[SYSCALL_LAST + 1] = {
	[SYSCALL_THREAD_EXIT] =		{ 0, 0, syscall_thread_exit },
	[SYSCALL_THREAD_CREATE] =	{ 1, 0, syscall_thread_create },

	[SYSCALL_CONSOLE_PUTC] =	{ 1, 0, syscall_console_putc },
	[SYSCALL_CONSOLE_PUTS] =	{ 2, 0, syscall_console_puts },
	[SYSCALL_CONSOLE_GETC] =	{ 0, 1, syscall_console_getc },

	[SYSCALL_IPC_PORT_ALLOCATE] =	{ 1, 1, syscall_ipc_port_allocate },
	[SYSCALL_IPC_PORT_SEND] =	{ 2, 0, syscall_ipc_port_send },
	[SYSCALL_IPC_PORT_WAIT] =	{ 1, 0, syscall_ipc_port_wait },
	[SYSCALL_IPC_PORT_RECEIVE] =	{ 2, 1, syscall_ipc_port_receive },
	[SYSCALL_IPC_TASK_PORT] =	{ 0, 1, syscall_ipc_task_port },

	[SYSCALL_VM_PAGE_GET] =		{ 0, 1, syscall_vm_page_get },
	[SYSCALL_VM_PAGE_FREE] =	{ 1, 0, syscall_vm_page_free },
	[SYSCALL_VM_ALLOC] =		{ 1, 1, syscall_vm_alloc },
	[SYSCALL_VM_FREE] =		{ 2, 0, syscall_vm_free },
	[SYSCALL_VM_ALLOC_RANGE] =	{ 2, 0, syscall_vm_alloc_range },
};

int
syscall(unsigned number, register_t *cnt, register_t *params)
{
	struct syscall_vector *sv;
	int error;

	if (number > SYSCALL_LAST) {
		*cnt = 0;
		return (ERROR_NOT_AVAILABLE);
	}

	sv = &syscall_vector[number];
	if (sv->sv_handler == NULL) {
		*cnt = 0;
		return (ERROR_NOT_AVAILABLE);
	}

	if (*cnt != sv->sv_inputs) {
		*cnt = 0;
		return (ERROR_ARG_COUNT);
	}

	error = sv->sv_handler(params);
	if (error != 0) {
		*cnt = 0;
		return (error);
	}

	*cnt = sv->sv_outputs;
	return (0);
}

static int
syscall_thread_exit(register_t *params)
{
	(void)params;
	thread_exit();
	/* NOTREACHED */
}

static int
syscall_thread_create(register_t *params)
{
	struct thread *td;
	struct task *task;
	void *entry;
	int error;

	entry = (void *)(vaddr_t)params[0];

	task = current_task();
	error = thread_create(&td, task, "XXX", THREAD_DEFAULT | THREAD_USTACK);
	if (error != 0)
		return (error);

	/*
	 * Set up userland trampoline.
	 */
	thread_set_upcall(td, cpu_thread_user_trampoline, entry);

	scheduler_thread_runnable(td);

	return (0);
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
	vaddr_t kvaddr, uvaddr;
	size_t len, o;
	int error;

	uvaddr = params[0];
	len = params[1];

	error = vm_wire(current_task()->t_vm, uvaddr, len, &kvaddr, &o, false);
	if (error != 0)
		return (error);

	kcputsn((void *)(uintptr_t)(kvaddr + o), len);

	error = vm_unwire(current_task()->t_vm, uvaddr, len, kvaddr);
	if (error != 0)
		panic("%s: couldn't unwire string: %m", __func__, error);

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
	ipc_port_flags_t flags;
	ipc_port_t port;
	int error;

	flags = params[0];

	error = ipc_port_allocate(&port, flags);
	if (error != 0)
		return (error);

	params[0] = port;
	return (0);
}

static int
syscall_ipc_port_send(register_t *params)
{
	const struct ipc_header *ipch;
	vaddr_t kvaddr, uvaddr;
	size_t len, o;
	void *page;
	int error;

	uvaddr = params[0];
	len = sizeof *ipch;
	page = (void *)(uintptr_t)params[1];

	error = vm_wire(current_task()->t_vm, uvaddr, len, &kvaddr, &o, false);
	if (error != 0)
		return (error);
	ipch = (const struct ipc_header *)(uintptr_t)(kvaddr + o);

	error = ipc_port_send(ipch, page);
	if (error != 0)
		return (error);

	error = vm_unwire(current_task()->t_vm, uvaddr, len, kvaddr);
	if (error != 0)
		panic("%s: couldn't unwire string: %m", __func__, error);

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
	struct ipc_header *ipch;
	vaddr_t kvaddr, uvaddr;
	ipc_port_t port;
	size_t len, o;
	void *page;
	int error;

	port = params[0];
	uvaddr = params[1];
	len = sizeof *ipch;

	error = vm_wire(current_task()->t_vm, uvaddr, len, &kvaddr, &o, false);
	if (error != 0)
		return (error);
	ipch = (struct ipc_header *)(uintptr_t)(kvaddr + o);

	error = ipc_port_receive(port, ipch, &page);
	if (error != 0)
		return (error);

	error = vm_unwire(current_task()->t_vm, uvaddr, len, kvaddr);
	if (error != 0)
		panic("%s: couldn't unwire string: %m", __func__, error);

	params[0] = (uintptr_t)page;

	return (0);
}

static int
syscall_ipc_task_port(register_t *params)
{
	struct thread *td;

	td = current_thread();

	params[0] = td->td_task->t_ipc.ipct_task_port;
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

static int
syscall_vm_alloc(register_t *params)
{
	struct thread *td;
	vaddr_t vaddr;
	int error;

	td = current_thread();

	error = vm_alloc(td->td_task->t_vm, (size_t)params[0], &vaddr);
	if (error != 0)
		return (error);
	params[0] = vaddr;

	return (0);
}

static int
syscall_vm_free(register_t *params)
{
	struct thread *td;
	int error;

	td = current_thread();

	error = vm_free(td->td_task->t_vm, (size_t)params[1], (vaddr_t)params[0]);
	if (error != 0)
		return (error);

	return (0);
}

static int
syscall_vm_alloc_range(register_t *params)
{
	struct vm *vm;
	size_t pages, o;
	vaddr_t begin, end;
	int error;

	begin = params[0];
	end = params[1];
	pages = PAGE_COUNT(end - begin);

	if (PAGE_OFFSET(begin) != 0 || PAGE_OFFSET(end) != 0 || end <= begin)
		return (ERROR_INVALID);

	vm = current_task()->t_vm;

	error = vm_alloc_range(vm, begin, end);
	if (error != 0)
		return (error);

	if (vm == &kernel_vm)
		panic("%s: can't wire from kernel to kernel.", __func__);

	error = vm_alloc_range(vm, begin, end);
	if (error != 0)
		return (error);

	for (o = 0; o < pages; o++) {
		error = page_alloc_map(vm, PAGE_FLAG_DEFAULT, begin + o * PAGE_SIZE);
		if (error != 0) /* XXX Free and unmap instead.  */
			panic("%s: page_alloc_map failed: %m", __func__, error);
	}

	return (0);
}

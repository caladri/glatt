#include <cpu/asm.h>
#include <core/syscall.h>

	.set noreorder

	.text

ENTRY(_start)
	dla	gp, _gp
	jal	mu_main
	nop
END(_start)

ENTRY(exit)
	li	v0, SYSCALL_EXIT
	li	v1, 0
	syscall
END(exit)

ENTRY(putchar)
	li	v0, SYSCALL_CONSOLE_PUTC
	li	v1, 1
	syscall
	jr	ra
	nop
END(putchar)

ENTRY(putsn)
	li	v0, SYSCALL_CONSOLE_PUTS
	li	v1, 2
	syscall
	jr	ra
	nop
END(putsn)

ENTRY(getchar)
	li	v0, SYSCALL_CONSOLE_GETC
	li	v1, 0
	syscall

	beqz	v1, 1f
	nop

	jr	ra
	move	v0, a0

1:	jr	ra
	li	v0, -1
END(getchar)

ENTRY(ipc_port_allocate)
	move	t0, a0
	move	a0, a1

	li	v0, SYSCALL_IPC_PORT_ALLOCATE
	li	v1, 1
	syscall

	beqz	v1, 1f
	nop

	sw	a0, 0(t0)

1:	jr	ra
	nop
END(ipc_port_allocate)

ENTRY(ipc_port_send)
	li	v0, SYSCALL_IPC_PORT_SEND
	li	v1, 2
	syscall
	jr	ra
	nop
END(ipc_port_send)

ENTRY(ipc_port_wait)
	li	v0, SYSCALL_IPC_PORT_WAIT
	li	v1, 1
	syscall
	jr	ra
	nop
END(ipc_port_wait)

ENTRY(ipc_port_receive)
	move	t0, a2

	li	v0, SYSCALL_IPC_PORT_RECEIVE
	li	v1, 2
	syscall

	beqz	v1, 1f
	nop

	beqz	t0, 1f
	nop

	/* 
	 * ipc_port_receive(port, &ipch, &page);
	 */
	sd	a0, 0(t0)

	/*
	 * ipc_port_receive(port, &ipch, NULL);
	 */
1:	jr	ra
	nop
END(ipc_port_receive)

ENTRY(ipc_task_port)
	li	v0, SYSCALL_IPC_TASK_PORT
	li	v1, 0
	syscall

	/* Cannot fail.  Just return the port.  */
	move	v0, a0

	jr	ra
	nop
END(ipc_task_port)

ENTRY(vm_page_get)
	move	t0, a0

	li	v0, SYSCALL_VM_PAGE_GET
	li	v1, 0
	syscall

	beqz	v1, 1f
	nop

	sd	a0, 0(t0)

1:	jr	ra
	nop
END(vm_page_get)

ENTRY(vm_page_free)
	li	v0, SYSCALL_VM_PAGE_FREE
	li	v1, 1
	syscall
	jr	ra
	nop
END(vm_page_free)

ENTRY(vm_alloc)
	move	t0, a0
	move	a0, a1

	li	v0, SYSCALL_VM_ALLOC
	li	v1, 1
	syscall

	beqz	v1, 1f
	nop

	sd	a0, 0(t0)

1:	jr	ra
	nop
END(vm_alloc)

ENTRY(vm_free)
	li	v0, SYSCALL_VM_FREE
	li	v1, 2
	syscall
	jr	ra
	nop
END(vm_free)

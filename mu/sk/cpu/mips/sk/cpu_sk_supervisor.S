#include <cpu/sk/asm.h>

	NOREORDER

	.text

ENTRY(supervisor)
	syscall
END(supervisor)

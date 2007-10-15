#include <cpu/sk/asm.h>

	NOREORDER

	.data

	.p2align 3
	.space 1024
SYMBOL(bootstack)

	.text
ENTRY(start)
	/*
	 * Zero the cause.
	 */
	mtc0	zero, CP0_CAUSE

	/*
	 * Enter 64-bit kernel mode.
	 * XXX With boot exception vectors, once we have a good way to switch.
	 */
	li	t0, /*CP0_STATUS_BEV | */CP0_STATUS_KX
	mtc0	t0, CP0_STATUS

	/*
	 * Switch to a temporary stack.
	 */
	dla	sp, bootstack

	/*
	 * Set the gp to 0 (since we are not using it.)
	 */
	move	gp, zero

	/*
	 * Initialize the supervisor/kernel.
	 */
	jal	sk_init
	nop

	/*
	 * Should not be reached.
	 */
	break
END(start)

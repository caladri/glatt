#include <cpu/sk/asm.h>

	NOREORDER

	.data
	.space 1024
SYMBOL(bootstack)

	.text
ENTRY(start)
	/*
	 * Enter 64-bit kernel mode with boot exception vectors.
	 */
	li	t0, CP0_STATUS_BEV | CP0_STATUS_KX
	mtc0	t0, CP0_STATUS

	/*
	 * Switch to a temporary stack.
	 */
	dla	sp, bootstack

	/*
	 * Zero the cause register and set the gp to 0 (since we are not using
	 * it.)
	 */
	mtc0	zero, CP0_CAUSE
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

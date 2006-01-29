#include <cpu/asm.h>

	NOREORDER

	.data
	.space 1024
SYMBOL(tempstack)

	.text
ENTRY(start)
	/*
	 * Preserve these bits if they're set.
	 */
	li	t0, CP0_STATUS_BEV | CP0_STATUS_SR

	/*
	 * Set these bits. XXX enable FPU and start it up!
	 */
	li	t1, CP0_STATUS_KX

	/*
	 * Read status register and fixup what bits are set.
	 */
	mfc0	t2, CP0_STATUS
	and	t2, t0
	or	t2, t1
	mtc0	t2, CP0_STATUS

	/* Clear cause register.  */
	mtc0	zero, CP0_CAUSE

	/* Switch to a temporary stack.  */
	dla	sp, tempstack

	/* Set up the GP.  */
	dla	gp, _gp

	/* Start platform rolling.  */
	jal	platform_start
	nop

	/* Go into main loop.  */
	//jal	main
	nop

	/* NOTREACHED */
END(start)

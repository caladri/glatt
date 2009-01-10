#include <cpu/asm.h>

	NOREORDER

	.data
	.space PAGE_SIZE
SYMBOL(tempstack)

	.text
ENTRY(start)
	/*
	 * Preserve these bits if they're set.
	 */
	li	t0, CP0_STATUS_SR

	/*
	 * Set these bits. XXX enable FPU and start it up!
	 */
	li	t1, CP0_STATUS_BEV | CP0_STATUS_KX

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

	/*
	 * We don't use the gp, set it to NULL.
	 */
	move	gp, zero

	/* Start everything from C.  */
	jal	platform_start
	nop

	/* NOTREACHED */
	break
END(start)

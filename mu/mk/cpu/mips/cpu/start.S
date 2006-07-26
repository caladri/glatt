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

	/* Start platform rolling.  */
	jal	platform_start
	nop

	/* Allocate a proper stack page and switch to it.  */
	/*
	 * We must be able to get by with one page of stack until threads are
	 * running.  At that point we'll have a stack of KSTACK_SIZE in a
	 * virtual address space.
	 */
1:	dla	a0, kernel_vm
	dsubu	sp, 8
	li	a1, PAGE_FLAG_DEFAULT
	jal	page_alloc_direct
	move	a2, sp
	daddu	sp, 8
	bnez	v0, 1b
	nop
	ld	t0, -8(sp)		/* Wait for it...  */
	daddu	t0, PAGE_SIZE		/* Wait for it...  */
	move	sp, t0			/* Bam!  */

	jal	platform_startup
	nop

	/* NOTREACHED */
END(start)

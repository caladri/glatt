#include <core/_macro.h>
#include <cpu/asm.h>
#include <cpu/memory.h>

	/* Vectors for exception, interrupt and TLB miss handling.  */
	.data
exception_table:
	.dword	generic_exception	/* Int */
	.dword	tlbmod_exception	/* TLBMod */
	.dword	tlb_exception		/* TLBL */
	.dword	tlb_exception		/* TLBS */
	.dword	generic_exception	/* AdEL */
	.dword	generic_exception	/* AdES */
	.dword	generic_exception	/* IBE */
	.dword	generic_exception	/* DBE */
	.dword	generic_exception	/* Sys */
	.dword	generic_exception	/* Bp */
	.dword	generic_exception	/* RI */
	.dword	generic_exception	/* CpU */
	.dword	generic_exception	/* Ov */
	.dword	generic_exception	/* Tr */
	.dword	generic_exception	/* VCEI */
	.dword	generic_exception	/* FPE */
	.dword	generic_exception	/* Res (16) */
	.dword	generic_exception	/* Res (17) */
	.dword	generic_exception	/* Res (18) */
	.dword	generic_exception	/* Res (19) */
	.dword	generic_exception	/* Res (20) */
	.dword	generic_exception	/* Res (21) */
	.dword	generic_exception	/* Res (22) */
	.dword	generic_exception	/* WATCH */
	.dword	generic_exception	/* Res (24) */
	.dword	generic_exception	/* Res (25) */
	.dword	generic_exception	/* Res (26) */
	.dword	generic_exception	/* Res (27) */
	.dword	generic_exception	/* Res (28) */
	.dword	generic_exception	/* Res (29) */
	.dword	generic_exception	/* Res (30) */
	.dword	cache_exception		/* VCED */

#define	VECTOR_ENTRY(v)							\
	ENTRY(CONCAT(v, _vector))

#define	VECTOR_END(v)							\
	END(CONCAT(v, _vector));					\
	SYMBOL(CONCAT(v, _vector_end))

	.text

	NOREORDER

VECTOR_ENTRY(utlb)
	.set noat
	break
	.set at
VECTOR_END(utlb)

VECTOR_ENTRY(exception)
	.set noat
	mfc0	k0, CP0_CAUSE
	dla	k1, exception_table

	and	k0, CP0_CAUSE_EXCEPTION
	dsrl	k0, CP0_CAUSE_EXCEPTION_SHIFT

	dsll	k0, 3	/* We're indexing an array of 64-bit values.  */
	daddu	k0, k1
	ld	k0, 0(k0)
	jr	k0
	nop
	.set at
VECTOR_END(exception)

VECTOR_ENTRY(xtlb)
	.set noat
	j	tlb_exception
	nop
	.set at
VECTOR_END(xtlb)

STATIC_ENTRY(generic_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop

1:	jal	exception
	move	a0, sp

	j	exception_frame_restore
	nop
	.set at
END(generic_exception)

STATIC_ENTRY(tlb_exception)
	.set noat
	dmfc0	k0, CP0_BADVADDR

	/*
	 * Check whether this is a kernel or user address.
	 */
	bltz	k0, 1f
	nop

	/*
	 * User address, load vm pointer.
	 */
	dli	k1, PCPU_VIRTUAL
	ld	k1, PC_THREAD(k1)
	teq	k1, zero		/* Must have a current thread.  */
	ld	k1, TD_TASK(k1)
	teq	k1, zero		/* Must have a parent task.  */
	ld	k1, T_VM(k1)
	teq	k1, zero		/* Must have a VM.  */

	b	2f			/* Continue.  */
	nop

1:	/*
	 * Kernel address -- bounds check and continue.
	 */
	dli	k1, KERNEL_END
	tgeu	k0, k1
	dla	k1, kernel_vm

2:	ld	k1, VM_PMAP(k1)
	daddu	k1, PM_LEVEL0

	/*
	 * PMAP -> L0.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PMAPL0SHIFT - 3
	andi	k0, PMAPL0MASK << 3
	daddu	k1, k0
	ld	k1, 0(k1)
	beqz	k1, 3f			/* L0 pointer is NULL.  */
	nop

	/*
	 * L0 -> L1.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, L1L0SHIFT - 3
	andi	k0, L1L0MASK << 3
	daddu	k1, k0
	ld	k1, 0(k1)
	beqz	k1, 3f			/* L1 pointer is NULL.  */
	nop

	/*
	 * L1 -> PTE.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PTEL1SHIFT - 3
	andi	k0, PTEL1MASK << 3
	daddu	k1, k0
	ld	k1, 0(k1)

	/*
	 * Check that the PTE is valid.
	 */
	andi	k0, k1, PG_V
	beqz	k0, 3f			/* Invalid PTE.  */
	nop

	/*
	 * Load the PTE into lo0, add the lo1 offset and load into lo1.
	 */
	dmtc0	k1, CP0_TLBENTRYLO0
	daddu	k1, TLB_LO1_PAGE_OFFSET
	dmtc0	k1, CP0_TLBENTRYLO1

	/*
	 * The CPU has set up an entryhi, check if it is in the TLB.
	 * XXX does the CPU also set index beforehand?
	 */
	tlbp
	/*
	 * If the entryhi _was_ already in the TLB, update it at its existing
	 * location.  Otherwise, write into a random one.
	 */
	mfc0	k0, CP0_TLBINDEX
	bltz	k0, 1f
	nop

	/* Found!  Indexed write.  */
	tlbwi
	eret

	/* Not found.  Random write.  */
1:	tlbwr
	eret

	/*
	 * Could not be handled here, go to C.
	 */
3:	j	generic_exception
	nop
	.set at
END(tlb_exception)

STATIC_ENTRY(tlbmod_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop

1:	jal	tlb_modify
	dmfc0	a0, CP0_BADVADDR

	j	exception_frame_restore
	nop
	.set at
END(tlbmod_exception)

STATIC_ENTRY(cache_exception)
	.set noat
	j	generic_exception
	nop
	.set at
END(cache_exception)

STATIC_ENTRY(exception_frame_save)		/* Return address is k0.  */
	.set noat
	mfc0	k1, CP0_STATUS
	andi	k1, CP0_STATUS_U
	beqz	k1, 1f
	nop

	/* User mode.  */

	/* Look up kernel stack.  */
	dli	k1, PCPU_VIRTUAL
	ld	k1, PC_THREAD(k1)
	teq	k1, zero		/* Must have a current thread.  */
	ld	k1, TD_KSTACK(k1)
	teq	k1, zero		/* Must have a kernel stack.  */

	/* Save and replace stack pointer.  */
	daddu	k1, KSTACK_SIZE - FRAME_SIZE	/* Frame at top of kstack.  */
	sd	sp, FRAME_SP(k1)
	move	sp, k1			/* Switch to kernel stack.  */

	/* Save status to return to after exception.  */
	li	k1, USER_STATUS
	b	2f			/* Continue with normal frame save.  */

	/* Kernel mode.  */
1:	dsubu	k1, sp, FRAME_SIZE
	sd	sp, FRAME_SP(k1)
	move	sp, k1

	mfc0	k1, CP0_STATUS
	andi	k1, CP0_STATUS_IE	/* Preserve interrupt status.  */
	ori	k1, KERNEL_STATUS

2:	sd	k1, FRAME_STATUS(sp)

	/*
	 * Finish saving state that would be clobbered by other exceptions
	 * and then drop EXL to 0 and enter kernel mode.  At that point we
	 * can take nested exceptions during save here.
	 */

	/* Save exception program counter.  */
	dmfc0	k1, CP0_EXCPC
	sd	k1, FRAME_EPC(sp)

	/* Save cause.  */
	mfc0	k1, CP0_CAUSE
	sd	k1, FRAME_CAUSE(sp)

	/* Enter kernel mode, EXL=0.  */
	mfc0	k1, CP0_STATUS
	andi	k1, CP0_STATUS_IE	/* Preserve interrupt status.  */
	ori	k1, KERNEL_STATUS
	mtc0	k1, CP0_STATUS		/* Enter kernel mode.  */

	sd	AT, FRAME_AT(sp)
	sd	v0, FRAME_V0(sp)
	sd	v1, FRAME_V1(sp)
	sd	a0, FRAME_A0(sp)
	sd	a1, FRAME_A1(sp)
	sd	a2, FRAME_A2(sp)
	sd	a3, FRAME_A3(sp)
	sd	a4, FRAME_A4(sp)
	sd	a5, FRAME_A5(sp)
	sd	a6, FRAME_A6(sp)
	sd	a7, FRAME_A7(sp)
	sd	t0, FRAME_T0(sp)
	sd	t1, FRAME_T1(sp)
	sd	t2, FRAME_T2(sp)
	sd	t3, FRAME_T3(sp)

	/* Use freshly-available temporaries to start loading externals.  */
	mfhi	t1
	mflo	t2

	sd	s0, FRAME_S0(sp)
	sd	s1, FRAME_S1(sp)
	sd	s2, FRAME_S2(sp)
	sd	s3, FRAME_S3(sp)
	sd	s4, FRAME_S4(sp)
	sd	s5, FRAME_S5(sp)
	sd	s6, FRAME_S6(sp)
	sd	s7, FRAME_S7(sp)
	sd	t8, FRAME_T8(sp)
	sd	t9, FRAME_T9(sp)
	sd	gp, FRAME_GP(sp)
	sd	s8, FRAME_S8(sp)
	sd	ra, FRAME_RA(sp)

	sd	t1, FRAME_HI(sp)
	sd	t2, FRAME_LO(sp)

	jr	k0
	nop
	.set at
END(exception_frame_save)

STATIC_ENTRY(exception_frame_restore)		/* Does not return.  */
	.set noat

	/* Enter kernel mode, EXL=1.  */
	li	k1, KERNEL_STATUS
	ori	k1, CP0_STATUS_EXL	/* Back in exception handler.  */
	mtc0	k1, CP0_STATUS		/* Enter kernel mode.  */

	ld	t2, FRAME_LO(sp)
	ld	t1, FRAME_HI(sp)
	ld	t0, FRAME_EPC(sp)

	ld	ra, FRAME_RA(sp)
	ld	s8, FRAME_S8(sp)
	ld	gp, FRAME_GP(sp)
	ld	t9, FRAME_T9(sp)
	ld	t8, FRAME_T8(sp)
	ld	s7, FRAME_S7(sp)
	ld	s6, FRAME_S6(sp)
	ld	s5, FRAME_S5(sp)
	ld	s4, FRAME_S4(sp)
	ld	s3, FRAME_S3(sp)
	ld	s2, FRAME_S2(sp)
	ld	s1, FRAME_S1(sp)
	ld	s0, FRAME_S0(sp)

	/* Before we nuke temporaries, restore externals.  */
	mtlo	t2
	mthi	t1
	dmtc0	t0, CP0_EXCPC

	ld	t3, FRAME_T3(sp)
	ld	t2, FRAME_T2(sp)
	ld	t1, FRAME_T1(sp)
	ld	t0, FRAME_T0(sp)
	ld	a7, FRAME_A7(sp)
	ld	a6, FRAME_A6(sp)
	ld	a5, FRAME_A5(sp)
	ld	a4, FRAME_A4(sp)
	ld	a3, FRAME_A3(sp)
	ld	a2, FRAME_A2(sp)
	ld	a1, FRAME_A1(sp)
	ld	a0, FRAME_A0(sp)
	ld	v1, FRAME_V1(sp)
	ld	v0, FRAME_V0(sp)
	ld	AT, FRAME_AT(sp)

	/* Get original status.  */
	ld	k0, FRAME_STATUS(sp)
	dli	k1, PCPU_VIRTUAL
	ld	k1, PC_INTERRUPT_MASK(k1)
	or	k0, k1			/* Set interrupt mask.  */

	/* Load original stack pointer.  */
	ld	sp, FRAME_SP(sp)

	/* Restore status.  */
	ori	k0, CP0_STATUS_EXL	/* Still in exception handler.  */
	mtc0	k0, CP0_STATUS

	/* Return from exception.  */
	eret
	.set at
END(exception_frame_restore)

#include <core/_macro.h>
#include <cpu/asm.h>

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
	mtc0	zero, CP0_STATUS
	li	k0, KERNEL_STATUS
	mtc0	k0, CP0_STATUS

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

ENTRY(generic_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop
1:	dla	a0, 0(sp)
	jal	exception
	nop
	j	exception_frame_restore
	nop
	.set at
END(generic_exception)

ENTRY(tlb_exception)
	.set noat
	mtc0	zero, CP0_STATUS
	li	k0, KERNEL_STATUS
	mtc0	k0, CP0_STATUS

	dmfc0	k0, CP0_BADVADDR
	dli	k1, KERNEL_END
	tgeu	k0, k1
	dli	k1, KERNEL_BASE
	tltu	k0, k1

	dla	k1, kernel_vm
	ld	k1, VM_PMAP(k1)
	daddu	k1, PM_LEVEL0

	/*
	 * PMAP -> L0.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PMAPL0SHIFT - 3
	andi	k0, PMAPL0MASK << 3
	daddu	k1, k0
	ld	k1, 0(k1)
	teq	zero, k1		/* L0 pointer can't be NULL.  */

	/*
	 * L0 -> L1.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, L1L0SHIFT - 3
	andi	k0, L1L0MASK << 3
	daddu	k1, k0
	ld	k1, 0(k1)
	teq	zero, k1		/* L1 pointer can't be NULL.  */

	/*
	 * L1 -> PTE.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PTEL1SHIFT - 3
	andi	k0, PTEL1MASK << 3
	daddu	k1, k0
	ld	k1, 0(k1)

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
	j	exception_return
	tlbwi

	/* Not found.  Random write.  */
1:	j	exception_return
	tlbwr
	.set at
END(tlb_exception)

ENTRY(tlbmod_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop
1:	dmfc0	a0, CP0_BADVADDR
	jal	tlb_modify
	nop
	j	exception_frame_restore
	nop
	.set at
END(tlbmod_exception)

ENTRY(cache_exception)
	.set noat
	j	generic_exception
	nop
	.set at
END(cache_exception)

ENTRY(exception_frame_save)		/* Return address is k0.  */
	.set noat
	dsubu	sp, FRAME_SIZE

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
	dmfc0	t0, CP0_EXCPC
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
	sd	sp, FRAME_SP(sp)
	sd	s8, FRAME_S8(sp)
	sd	ra, FRAME_RA(sp)

	sd	t0, FRAME_EPC(sp)
	sd	t1, FRAME_HI(sp)
	sd	t2, FRAME_LO(sp)

	li	k1, KERNEL_STATUS

	jr	k0
	mtc0	k1, CP0_STATUS
	.set at
END(exception_frame_save)

ENTRY(exception_frame_restore)		/* Does not return.  */
	.set noat
	ld      t2, FRAME_LO(sp)
	ld      t1, FRAME_HI(sp)
	ld      t0, FRAME_EPC(sp)

	ld      ra, FRAME_RA(sp)
	ld      s8, FRAME_S8(sp)
	ld      sp, FRAME_SP(sp)
	ld      gp, FRAME_GP(sp)
	ld      t9, FRAME_T9(sp)
	ld      t8, FRAME_T8(sp)
	ld      s7, FRAME_S7(sp)
	ld      s6, FRAME_S6(sp)
	ld      s5, FRAME_S5(sp)
	ld      s4, FRAME_S4(sp)
	ld      s3, FRAME_S3(sp)
	ld      s2, FRAME_S2(sp)
	ld      s1, FRAME_S1(sp)
	ld      s0, FRAME_S0(sp)

	/* Before we nuke temporaries, restore externals.  */
	mtlo	t2
	mthi	t1
	dmtc0	t0, CP0_EXCPC

	ld      t3, FRAME_T3(sp)
	ld      t2, FRAME_T2(sp)
	ld      t1, FRAME_T1(sp)
	ld      t0, FRAME_T0(sp)
	ld      a7, FRAME_A7(sp)
	ld      a6, FRAME_A6(sp)
	ld      a5, FRAME_A5(sp)
	ld      a4, FRAME_A4(sp)
	ld      a3, FRAME_A3(sp)
	ld      a2, FRAME_A2(sp)
	ld      a1, FRAME_A1(sp)
	ld      a0, FRAME_A0(sp)
	ld      v1, FRAME_V1(sp)
	ld      v0, FRAME_V0(sp)
	ld      AT, FRAME_AT(sp)

	daddu	sp, FRAME_SIZE

	j	exception_return
	nop
	.set at
END(exception_frame_restore)

ENTRY(exception_return)
	.set noat
	/* XXX userland.  */
	dmfc0	k0, CP0_EXCPC
	dla	k1, exception_return_fixup
	dmtc0	k1, CP0_EXCPC

	eret

	/* Should not be reached.  */
	break
exception_return_fixup:
	/*
	 * Restore kernel status, enable interrupts if needed.
	 */
	dla	k1, PCPU_VIRTUAL
	ld	k1, PC_INTERRUPT_ENABLE(k1)
	beqz	k1, 1f
	nop

	/* Interrupts enabled.  */
	dla	k1, PCPU_VIRTUAL
	ld	k1, PC_INTERRUPT_MASK(k1)
	or	k1, CP0_STATUS_IE
	b	2f
	nop

1:	/* Interrupts disabled.  */
	dla	k1, PCPU_VIRTUAL
	ld	k1, PC_INTERRUPT_MASK(k1)
	/* Fallthrough.  */

2:	/* Set the final bits and return from interrupt.  */
	or	k1, KERNEL_STATUS
	j	k0
	mtc0	k1, CP0_STATUS
	.set at
END(exception_return)

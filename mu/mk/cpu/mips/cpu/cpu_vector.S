#include <core/_macro.h>
#include <cpu/asm.h>

	/* Vectors for exception, interrupt and TLB miss handling.  */

	/*
	 * XXX TODO
	 * We have to allow for:
	 *
	 * 	(1) Exception nesting.
	 *	(2) TLB refills for the kernel stack.
	 *	(3) TLB refills for kernel address space in exception handlers.
	 *
	 * (2) and (3) mean that we must avoid touching virtual addresses in
	 * the TLB refill (XTLB) handler.  This is pretty easy to do here.
	 * Access violations will be handled elsewhere.  We can do a simple
	 * range check and panic if the address isn't in the kernel range for
	 * XTLB refill.
	 *
	 * (1) and (2) and (3) mean that we have to be able to handle nested
	 * exceptions.  In practice, this just means that until we have
	 * disabled exceptions and interrupts, we must not assume any state
	 * will be consistent unless we know it is.  Traps from user code, for
	 * example, will always require us to switch to a kstack, which is to
	 * be expected.  Kernel exception handlers could rely on growing the
	 * kernel stack downward, but that doesn't do us any favors if we start
	 * having recursive exceptions (we can underrun the stack allocation,
	 * which we want to be relatively small), and if sp gets invalidated,
	 * we will get into a very nasty state.
	 *
	 * Eventually we should be careful with k0 and k1 and have exceptions
	 * return with them in the same state as any previous exception handler
	 * would have expected.  For now, it's probably best we just take the
	 * easy route of growing the stack down.  In time, it would be a lot
	 * better to have a per-CPU exception stack which we switch to, and to
	 * catch recursion.
	 *
	 * There is a problem where we can be trying to service TLBMod for the
	 * kernel stack, and then we will end up trying to write to the stack
	 * to save the frame, which will recursively fault until we grow the
	 * stack down below where it's good to.  We will zero the kstack to make
	 * sure it's all dirty before we start running on it.
	 */

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

ENTRY(generic_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop
1:	dla	a0, 0(sp)
	jal	exception
	nop
	dla	k0, 1f
	j	exception_frame_restore
	nop
1:	eret
	.set at
END(generic_exception)

/*
 * XXX until we export SHIFT instead of DIV for each level, we are stuck
 * with all these silly shifts and hard-coding to 10.
 */
ENTRY(tlb_exception)
	.set noat
	dmfc0	k0, CP0_BADVADDR
	dli	k1, KERNEL_END
	tgeu	k0, k1
	dli	k1, KERNEL_BASE
	tltu	k0, k1

	/*
	 * PMAP -> L0.
	 */
	dmfc0	k0, CP0_BADVADDR
	dla	k1, kernel_vm
	ld	k1, VM_PMAP(k1)
	daddu	k1, PM_LEVEL0
	/* For now we just have 1 Level0.  */
	ld	k1, 0(k1)
	teq	zero, k1		/* L0 pointer can't be NULL.  */

	/*
	 * L0 -> L1.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PAGE_SHIFT		/* Get a page number.  */
	dsrl	k0, 10			/* Skip L2 -> PTE index.  */
	dsrl	k0, 10			/* Skip L1 -> L2  index.  */
	andi	k0, 0x3ff		/* We just want the low 10 bits.  */
	dsll	k0, 3			/* Get an 8-byte offset.  */
	daddu	k1, k0
	ld	k1, 0(k1)
	teq	zero, k1		/* L1 pointer can't be NULL.  */

	/*
	 * L1 -> L2.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PAGE_SHIFT		/* Get a page number.  */
	dsrl	k0, 10			/* Skip L2 -> PTE index.  */
	andi	k0, 0x3ff		/* We just want the low 10 bits.  */
	dsll	k0, 3			/* Get an 8-byte offset.  */
	daddu	k1, k0
	ld	k1, 0(k1)
	teq	zero, k1		/* L2 pointer can't be NULL.  */

	/*
	 * L2 -> PTE.
	 */
	dmfc0	k0, CP0_BADVADDR
	dsrl	k0, PAGE_SHIFT		/* Get a page number.  */
	andi	k0, 0x3ff		/* We just want the low 10 bits.  */
	dsll	k0, 3			/* Get an 8-byte offset.  */
	daddu	k1, k0
	ld	k1, 0(k1)

	/*
	 * Load the PTE into lo0, add the lo1 offset and load into lo1.  Check
	 * if the entryhi that the CPU set up is already in the TLB.  If so,
	 * write at that index, otherwise write to a random one.
	 */
	dmtc0	k1, CP0_TLBENTRYLO0
	daddu	k1, TLB_LO1_PAGE_OFFSET
	dmtc0	k1, CP0_TLBENTRYLO1
	tlbp
	dmfc0	k1, CP0_TLBINDEX
	bltz	k1, 1f
	nop

	tlbwi
	eret

1:	tlbwr
	eret
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
	dla	k0, 1f
	j	exception_frame_restore
	nop
1:	eret
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
	mfc0	t0, CP0_STATUS
	dmfc0	t1, CP0_EXCPC
	mfhi	t2
	mflo	t3

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

	sd	t0, FRAME_STATUS(sp)
	sd	t1, FRAME_EPC(sp)
	sd	t2, FRAME_HI(sp)
	sd	t3, FRAME_LO(sp)

	jr	k0
	nop
	.set at
END(exception_frame_save)

ENTRY(exception_frame_restore)		/* Return address is k0.  */
	.set noat
	ld      t3, FRAME_LO(sp)
	ld      t2, FRAME_HI(sp)
	ld      t1, FRAME_EPC(sp)
	ld      t0, FRAME_STATUS(sp)

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
	mtlo	t3
	mthi	t2
	dmtc0	t1, CP0_EXCPC
	mtc0	t0, CP0_STATUS

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

	jr	k0
	nop
	.set at
END(exception_frame_restore)

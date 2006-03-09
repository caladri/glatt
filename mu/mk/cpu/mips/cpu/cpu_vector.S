#include <core/_macro.h>
#include <cpu/asm.h>
#include <cpu/memory.h>

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

#define	KREGS_SAVE							\
	dsubu	sp, 8 ;							\
	sd	k0, 0(sp) ;						\
	dsubu	sp, 8 ;							\
	sd	k1, 0(sp)

#define	KREGS_RESTORE							\
	ld	k1, 0(sp) ;						\
	daddu	sp, 8 ;							\
	ld	k0, 0(sp) ;						\
	daddu	sp, 8

	.text

	NOREORDER

VECTOR_ENTRY(exception)
	.set noat
	KREGS_SAVE;
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

	/*
	 * XXX we could get an xtlb miss for the kernel stack.  We should try
	 * to handle TLB refills here without touching the stack.  Yow!
	 */
VECTOR_ENTRY(xtlb)
	.set noat
	KREGS_SAVE;
	j	tlb_exception
	nop
	.set at
VECTOR_END(xtlb)

ENTRY(generic_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop
1:	jal	exception
	nop
	dla	k0, 1f
	j	exception_frame_restore
	nop
1:	KREGS_RESTORE;
	eret
	.set at
END(generic_exception)

ENTRY(tlb_exception)
	.set noat
	dla	k0, 1f
	j	exception_frame_save
	nop
1:	dmfc0	a0, CP0_BADVADDR
	jal	tlb_refill
	nop
	dla	k0, 1f
	j	exception_frame_restore
	nop
1:	KREGS_RESTORE;
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
1:	KREGS_RESTORE;
	eret
	.set at
END(tlbmod_exception)

ENTRY(cache_exception)
	.set noat
	j	generic_exception
	nop
//	KREGS_RESTORE;
//	eret
	.set at
END(cache_exception)

ENTRY(exception_frame_save)		/* Return address is k0.  */
	.set noat
	dla	k1, PCPU_VIRTUAL	/* We assume the frame is first.  */
	dla	k1, PC_FRAME(k1)

	sd	AT, FRAME_AT(k1)
	sd	v0, FRAME_V0(k1)
	sd	v1, FRAME_V1(k1)
	sd	a0, FRAME_A0(k1)
	sd	a1, FRAME_A1(k1)
	sd	a2, FRAME_A2(k1)
	sd	a3, FRAME_A3(k1)
	sd	a4, FRAME_A4(k1)
	sd	a5, FRAME_A5(k1)
	sd	a6, FRAME_A6(k1)
	sd	a7, FRAME_A7(k1)
	sd	t0, FRAME_T0(k1)
	sd	t1, FRAME_T1(k1)
	sd	t2, FRAME_T2(k1)
	sd	t3, FRAME_T3(k1)

	/* Use freshly-available temporaries to start loading externals.  */
	mfc0	t0, CP0_STATUS
	dmfc0	t1, CP0_EXCPC
	mfhi	t2
	mflo	t3

	sd	s0, FRAME_S0(k1)
	sd	s1, FRAME_S1(k1)
	sd	s2, FRAME_S2(k1)
	sd	s3, FRAME_S3(k1)
	sd	s4, FRAME_S4(k1)
	sd	s5, FRAME_S5(k1)
	sd	s6, FRAME_S6(k1)
	sd	s7, FRAME_S7(k1)
	sd	t8, FRAME_T8(k1)
	sd	t9, FRAME_T9(k1)
	sd	gp, FRAME_GP(k1)
	sd	sp, FRAME_SP(k1)
	sd	s8, FRAME_S8(k1)
	sd	ra, FRAME_RA(k1)

	sd	t0, FRAME_STATUS(k1)
	sd	t1, FRAME_EPC(k1)
	sd	t2, FRAME_HI(k1)
	sd	t3, FRAME_LO(k1)

	jr	k0
	nop
	.set at
END(exception_frame_save)

ENTRY(exception_frame_restore)		/* Return address is k0.  */
	.set noat
	dla	k1, PCPU_VIRTUAL	/* We assume the frame is first.  */
	dla	k1, PC_FRAME(k1)

	ld      t3, FRAME_LO(k1)
	ld      t2, FRAME_HI(k1)
	ld      t1, FRAME_EPC(k1)
	ld      t0, FRAME_STATUS(k1)

	ld      ra, FRAME_RA(k1)
	ld      s8, FRAME_S8(k1)
	ld      sp, FRAME_SP(k1)
	ld      gp, FRAME_GP(k1)
	ld      t9, FRAME_T9(k1)
	ld      t8, FRAME_T8(k1)
	ld      s7, FRAME_S7(k1)
	ld      s6, FRAME_S6(k1)
	ld      s5, FRAME_S5(k1)
	ld      s4, FRAME_S4(k1)
	ld      s3, FRAME_S3(k1)
	ld      s2, FRAME_S2(k1)
	ld      s1, FRAME_S1(k1)
	ld      s0, FRAME_S0(k1)

	/* Before we nuke temporaries, restore externals.  */
	mtlo	t3
	mthi	t2
	dmtc0	t1, CP0_EXCPC
	mtc0	t0, CP0_STATUS

	ld      t3, FRAME_T3(k1)
	ld      t2, FRAME_T2(k1)
	ld      t1, FRAME_T1(k1)
	ld      t0, FRAME_T0(k1)
	ld      a7, FRAME_A7(k1)
	ld      a6, FRAME_A6(k1)
	ld      a5, FRAME_A5(k1)
	ld      a4, FRAME_A4(k1)
	ld      a3, FRAME_A3(k1)
	ld      a2, FRAME_A2(k1)
	ld      a1, FRAME_A1(k1)
	ld      a0, FRAME_A0(k1)
	ld      v1, FRAME_V1(k1)
	ld      v0, FRAME_V0(k1)
	ld      AT, FRAME_AT(k1)

	jr	k0
	nop
	.set at
END(exception_frame_restore)

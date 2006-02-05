#include <core/_macro.h>
#include <cpu/asm.h>
#include <cpu/memory.h>

	/* Vectors for exception, interrupt and TLB miss handling.  */

	.data
exception_table:
	.dword	generic_exception	/* Int */
	.dword	generic_exception	/* TLBMod */
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

#define	SAVE(r, d)							\
	sd	$r, (r * 8)(d)

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
	dla	k1, 1f
	jr	k0
	nop
1:	KREGS_RESTORE;
	eret
	.set at
VECTOR_END(exception)

VECTOR_ENTRY(xtlb)
	.set noat
	KREGS_SAVE;
	dla	k1, 1f
	j	tlb_exception
	nop
1:	KREGS_RESTORE;
	eret
	.set at
VECTOR_END(xtlb)

ENTRY(generic_exception)
1:	b	1b
	nop
END(generic_exception)

ENTRY(tlb_exception)
1:	b	1b
	nop
END(tlb_exception)

ENTRY(cache_exception)
1:	b	1b
	nop
END(cache_exception)

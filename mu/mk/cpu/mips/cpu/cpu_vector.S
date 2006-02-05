#include <core/_macro.h>
#include <cpu/asm.h>
#include <cpu/memory.h>

	/* Vectors for exception, interrupt and TLB miss handling.  */

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

	NOREORDER

VECTOR_ENTRY(exception)
	.set noat
	KREGS_SAVE;
	KREGS_RESTORE;
	.set at
VECTOR_END(exception)

VECTOR_ENTRY(xtlb)
	.set noat
	KREGS_SAVE;
	KREGS_RESTORE;
	.set at
VECTOR_END(xtlb)

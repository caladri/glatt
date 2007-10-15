#include <cpu/sk/asm.h>
#include <cpu/sk/exception.h>

	NOREORDER

	.text

VECTOR_ENTRY(Generic)
	mtc0	zero, CP0_STATUS

	dsubu	sp, 8
	sd	ra, 0(sp)

	dsubu	sp, 8
	mfc0	ra, CP0_EXCPC
	sd	ra, 0(sp)

	li	k0, CP0_STATUS_KX
	mtc0	k0, CP0_STATUS

	jal	supervisor_invoke
	nop

	ld	ra, 0(sp)
	mtc0	ra, CP0_EXCPC
	daddu	sp, 8

	ld	ra, 0(sp)
	daddu	sp, 8

	eret
VECTOR_END(Generic)

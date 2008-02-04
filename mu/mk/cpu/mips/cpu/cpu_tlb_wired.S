#include <core/_macro.h>
#include <cpu/asm.h>

	NOREORDER

ENTRY(tlb_wired_load)
	dla	t0, TWC_ENTRIES(a0)

	move	t1, zero
	move	t2, t0
next:	ld	t3, TWE_ENTRYHI(t2)
	beqz	t3, skip
	nop
	dmtc0	t3, CP0_TLBENTRYHI
	ld	t3, TWE_ENTRYLO0(t2)
	dmtc0	t3, CP0_TLBENTRYLO0
	ld	t3, TWE_ENTRYLO1(t2)
	dmtc0	t3, CP0_TLBENTRYLO1
	move	t3, t1
	daddu	t3, TLB_WIRED_BASE
	dmtc0	t3, CP0_TLBINDEX
	tlbwi
	
	move	t3, t1
	daddu	t3, TLB_WIRED_BASE
	daddu	t3, 1
	mtc0	t3, CP0_TLBWIRED

skip:	daddu	t1, 1
	daddu	t2, (8 * 3) /* Next entry.  */
	dli	t3, TLB_WIRED_COUNT
	blt	t1, t3, next
	nop

	jr	ra
	nop
END(tlb_wired_load)

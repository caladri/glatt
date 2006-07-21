#include <core/_macro.h>
#include <cpu/asm.h>

	/*
	 * void cpu_context_restore(struct thread *a0);
	 * t0 = thread.
	 * t1 = context.
	 * t2 = status.
	 * t3 = pcpu.
	 */
ENTRY(cpu_context_restore)
	move	t0, a0
	dla	t1, TD_CONTEXT(t0)
	ld	s0, CONTEXT_S0(t1)
	ld	s1, CONTEXT_S1(t1)
	ld	s2, CONTEXT_S2(t1)
	ld	s3, CONTEXT_S3(t1)
	ld	s4, CONTEXT_S4(t1)
	ld	s5, CONTEXT_S5(t1)
	ld	s6, CONTEXT_S6(t1)
	ld	s7, CONTEXT_S7(t1)
	ld	s8, CONTEXT_S8(t1)
	ld	sp, CONTEXT_SP(t1)
	ld	t2, CONTEXT_STATUS(t1)
	ld	ra, CONTEXT_RA(t1)
	/*
	 * Copy saved registers into argument registers for upcalls.
	 */
	move	a0, s0
	move	a1, s1
	move	a2, s2
	move	a3, s3
	move	a4, s4
	move	a5, s5
	move	a6, s6
	move	a7, s7
	/*
	 * Set thread pointer.
	 */
	dla	t3, PCPU_VIRTUAL
	sd	t0, PC_THREAD(t3)
	/*
	 * Get interrupt mask.
	 */
	ld	t0, PC_INTERRUPT_MASK(t3)
	/*
	 * We'll be returning true from cpu_context_save().
	 */
	li	v0, TRUE
	/*
	 * Clear interrpt mask in status.  Set interrupt mask from PCPU.
	 */
	and	t2, ~CP0_STATUS_INTERRUPT_MASK
	or	t2, t0
	/*
	 * Restore status register and jump to return address.
	 */
	mtc0	t2, CP0_STATUS
	jr	ra
END(cpu_context_restore)

	/*
	 * bool cpu_context_save(struct thread *a0);
	 * Returns true on restore, false on save.
	 */
ENTRY(cpu_context_save)
	move	t0, a0
	dla	t0, TD_CONTEXT(a0)
	mfc0	t1, CP0_STATUS
	sd	s0, CONTEXT_S0(t0)
	sd	s1, CONTEXT_S1(t0)
	sd	s2, CONTEXT_S2(t0)
	sd	s3, CONTEXT_S3(t0)
	sd	s4, CONTEXT_S4(t0)
	sd	s5, CONTEXT_S5(t0)
	sd	s6, CONTEXT_S6(t0)
	sd	s7, CONTEXT_S7(t0)
	sd	s8, CONTEXT_S8(t0)
	sd	sp, CONTEXT_SP(t0)
	sd	t1, CONTEXT_STATUS(t0)
	sd	ra, CONTEXT_RA(t0)
	/*
	 * Return false to indicate this isn't a restore.
	 */
	li	v0, FALSE
	jr	ra
END(cpu_context_save)

#include <core/_macro.h>
#include <cpu/asm.h>
#include <cpu/memory.h>

	NOREORDER

	/*
	 * void cpu_context_restore(struct thread *a0);
	 * t0 = thread.
	 * t1 = context.
	 * t2 = pcpu.
	 */
ENTRY(cpu_context_restore)
	/*
	 * XXX Deactivate any previous thread.
	 */

	move	s0, a0
	dla	a0, TD_CPUTHREAD(a0)
	dla	a0, TD_TLBWIRED(a0)
	jal	tlb_wired_load
	nop

	/*
	 * Activate this thread.
	 */
	jal	cpu_thread_activate
	move	a0, s0

	/*
	 * Load context.
	 */
	move	t0, s0
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
	 * We'll be returning true from cpu_context_save().
	 */
	jr	ra
	li	v0, TRUE
END(cpu_context_restore)

	/*
	 * bool cpu_context_save(struct thread *a0);
	 * Returns true on restore, false on save.
	 */
ENTRY(cpu_context_save)
	dla	t0, TD_CONTEXT(a0)
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
	sd	ra, CONTEXT_RA(t0)

	/*
	 * Return false to indicate this isn't a restore.
	 */
	jr	ra
	li	v0, FALSE
END(cpu_context_save)

	/*
	 * void cpu_thread_trampoline(struct thread *a0, void (*a1)(struct thread *, void *), void *a2)
	 * Begin running a1(a0, a2) in a thread.
	 * NB: s[0-7] alias a[0-7] so no need to save inbound arguments.
	 */
ENTRY(cpu_thread_trampoline)
	jal	scheduler_activate
	nop
	move	a0, s0
	jr	s1
	move	a1, s2
END(cpu_thread_trampoline)

#ifdef EXEC
	/*
	 * void cpu_thread_trampoline_user(struct thread *a0, void (*a1)(void *), void *a2)
	 * Begin running a1(a2) in a user thread.
	 * NB: s[0-7] alias a[0-7] so no need to save inbound arguments.
	 */
ENTRY(cpu_thread_trampoline_user)
	jal	scheduler_activate
	nop

	ld	t2, TD_USTACK_TOP(s0)
	teq	t2, zero

	dli	t0, PCPU_VIRTUAL
	ld	t0, PC_INTERRUPT_MASK(t0)
	li	t1, USER_STATUS
	or	t0, t1
	ori	t0, CP0_STATUS_EXL
	mtc0	t0, CP0_STATUS

	/* Now that we've set user status, switch to user stack.  */
	move	sp, t2
	/* Cannot take a fault beyond this point!  */

	move	a0, s2

	dmtc0	s1, CP0_EXCPC
	eret
END(cpu_thread_trampoline_user)
#endif

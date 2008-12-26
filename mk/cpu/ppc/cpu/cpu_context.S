#include <core/_macro.h>
#include <cpu/asm.h>

	/*
	 * void cpu_context_restore(struct thread *a0);
	 * t0 = thread.
	 * t1 = context.
	 * t2 = pcpu.
	 */
ENTRY(cpu_context_restore)
END(cpu_context_restore)

	/*
	 * bool cpu_context_save(struct thread *a0);
	 * Returns true on restore, false on save.
	 */
ENTRY(cpu_context_save)
END(cpu_context_save)

#include <core/types.h>
#include <core/error.h>
#include <core/syscall.h>
#include <cpu/frame.h>
#include <cpu/syscall.h>

#include <cpu/memory.h>
#include <core/console.h>

void
cpu_syscall(struct frame *frame)
{
#if 0
	unsigned i;

	for (i = 0; i < FRAME_COUNT; i++)
		printf(">sys frame[%x] = %#jx\n", i, (uintmax_t)frame->f_regs[i]);
	printf("stack %p\n", (void *)frame->f_regs[FRAME_SP]);
	for (i = 0; i < 16; i++) {
		vaddr_t sp = frame->f_regs[FRAME_SP] + i * sizeof (uintmax_t);
		if (sp > USER_STACK_TOP) {
			printf("stack top\n");
			break;
		}
		printf("stack[%u x u64] = %#jx\n", i, *(const uint64_t *)sp);
	}
#endif

	if (frame->f_regs[FRAME_V1] > 4) {
		/*
		 * If there are more than 4 arguments to this syscall, error.
		 */
		frame->f_regs[FRAME_V0] = ERROR_INVALID;
	} else {
		frame->f_regs[FRAME_V0] = syscall(frame->f_regs[FRAME_V0], &frame->f_regs[FRAME_V1], &frame->f_regs[FRAME_A0]);
	}

	frame->f_regs[FRAME_EPC] += 4;

#if 0
	for (i = 0; i < FRAME_COUNT; i++)
		printf("<sys frame[%x] = %#jx\n", i, (uintmax_t)frame->f_regs[i]);

	printf("stack %p\n", (void *)frame->f_regs[FRAME_SP]);
	for (i = 0; i < 16; i++) {
		vaddr_t sp = frame->f_regs[FRAME_SP] + i * sizeof (uintmax_t);
		if (sp > USER_STACK_TOP) {
			printf("stack top\n");
			break;
		}
		printf("stack[%u x u64] = %#jx\n", i, *(const uint64_t *)sp);
	}
#endif
}

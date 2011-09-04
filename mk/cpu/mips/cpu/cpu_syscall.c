#include <core/types.h>
#include <core/error.h>
#include <core/syscall.h>
#include <cpu/frame.h>
#include <cpu/syscall.h>

void
cpu_syscall(struct frame *frame)
{
	if (frame->f_regs[FRAME_V1] > 4) {
		/*
		 * If there are more than 4 arguments to this syscall, error.
		 */
		frame->f_regs[FRAME_V0] = ERROR_INVALID;
	} else {
		frame->f_regs[FRAME_V0] = syscall(frame->f_regs[FRAME_V0], &frame->f_regs[FRAME_V1], &frame->f_regs[FRAME_A0]);
	}
	frame->f_regs[FRAME_EPC] += 4;
}

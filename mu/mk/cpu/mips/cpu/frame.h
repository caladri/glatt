#ifndef	_CPU_FRAME_H_
#define	_CPU_FRAME_H_

#include <cpu/register.h>

struct frame {
	register_t f_regs[32];
};

#endif /* !_CPU_FRAME_H_ */

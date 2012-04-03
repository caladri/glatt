#ifndef	_CPU_FRAME_H_
#define	_CPU_FRAME_H_

#include <cpu/register.h>

	/* A frame for everything for system call handling and debugging.  */

#define	FRAME_COUNT	(35)

struct frame {
	register_t f_regs[FRAME_COUNT];
};

#define	FRAME_AT	(0x00)
#define	FRAME_V0	(0x01)
#define	FRAME_V1	(0x02)
#define	FRAME_A0	(0x03)
#define	FRAME_A1	(0x04)
#define	FRAME_A2	(0x05)
#define	FRAME_A3	(0x06)
#define	FRAME_A4	(0x07)
#define	FRAME_A5	(0x08)
#define	FRAME_A6	(0x09)
#define	FRAME_A7	(0x0a)
#define	FRAME_T0	(0x0b)
#define	FRAME_T1	(0x0c)
#define	FRAME_T2	(0x0d)
#define	FRAME_T3	(0x0e)
#define	FRAME_S0	(0x0f)
#define	FRAME_S1	(0x10)
#define	FRAME_S2	(0x11)
#define	FRAME_S3	(0x12)
#define	FRAME_S4	(0x13)
#define	FRAME_S5	(0x14)
#define	FRAME_S6	(0x15)
#define	FRAME_S7	(0x16)
#define	FRAME_T8	(0x17)
#define	FRAME_T9	(0x18)
#define	FRAME_GP	(0x19)
#define	FRAME_SP	(0x1a)
#define	FRAME_S8	(0x1b)
#define	FRAME_RA	(0x1c)
#define	FRAME_EPC	(0x1d)
#define	FRAME_HI	(0x1e)
#define	FRAME_LO	(0x1f)
#define	FRAME_STATUS	(0x20)
#define	FRAME_CAUSE	(0x21)
#define	FRAME_BADVADDR	(0x22)

#endif /* !_CPU_FRAME_H_ */

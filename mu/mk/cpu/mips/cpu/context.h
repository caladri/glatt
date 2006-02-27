#ifndef	_CPU_CONTEXT_H_
#define	_CPU_CONTEXT_H_

#include <cpu/register.h>

#define	CONTEXT_COUNT	(12)

struct context {
	register_t c_regs[CONTEXT_COUNT];
};

#define	CONTEXT_S0	(0x00)
#define	CONTEXT_S1	(0x01)
#define	CONTEXT_S2	(0x02)
#define	CONTEXT_S3	(0x03)
#define	CONTEXT_S4	(0x04)
#define	CONTEXT_S5	(0x05)
#define	CONTEXT_S6	(0x06)
#define	CONTEXT_S7	(0x07)
#define	CONTEXT_S8	(0x08)
#define	CONTEXT_SP	(0x09)
#define	CONTEXT_STATUS	(0x0a)
#define	CONTEXT_RA	(0x0b)

#endif /* !_CPU_CONTEXT_H_ */

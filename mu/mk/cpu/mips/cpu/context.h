#ifndef	_CPU_CONTEXT_H_
#define	_CPU_CONTEXT_H_

#include <cpu/register.h>

struct thread;

#define	CONTEXT_COUNT	(11)

struct context {
	register_t c_regs[CONTEXT_COUNT];
};

#define	CONTEXT_S0	(0x00)		/* Copied to a0.  */
#define	CONTEXT_S1	(0x01)		/* Copied to a1.  */
#define	CONTEXT_S2	(0x02)		/* Copied to a2.  */
#define	CONTEXT_S3	(0x03)		/* Copied to a3.  */
#define	CONTEXT_S4	(0x04)		/* Copied to a4.  */
#define	CONTEXT_S5	(0x05)		/* Copied to a5.  */
#define	CONTEXT_S6	(0x06)		/* Copied to a6.  */
#define	CONTEXT_S7	(0x07)		/* Copied to a7.  */
#define	CONTEXT_S8	(0x08)
#define	CONTEXT_SP	(0x09)
#define	CONTEXT_RA	(0x0a)

void cpu_context_restore(struct thread *);
bool cpu_context_save(struct thread *);
void cpu_context_switch(struct thread *, struct thread *);

#endif /* !_CPU_CONTEXT_H_ */

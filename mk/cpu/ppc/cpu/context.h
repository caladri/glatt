#ifndef	_CPU_CONTEXT_H_
#define	_CPU_CONTEXT_H_

#include <cpu/register.h>

struct thread;

#define	CONTEXT_COUNT	(11)

struct context {
	register_t c_regs[CONTEXT_COUNT];
};

void cpu_context_restore(struct thread *);
bool cpu_context_save(struct thread *);
void cpu_context_switch(struct thread *, struct thread *);

#endif /* !_CPU_CONTEXT_H_ */

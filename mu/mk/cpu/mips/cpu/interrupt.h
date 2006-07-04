#ifndef	_CPU_INTERRUPT_H_
#define	_CPU_INTERRUPT_H_

#include <cpu/register.h>

#define	CPU_INTERRUPT_COUNT	(8)

typedef	void (interrupt_t)(void *, int);

struct interrupt_handler {
	interrupt_t *ih_func;
	void *ih_arg;
};

void cpu_interrupt_establish(int, interrupt_t *, void *);

void cpu_interrupt(void);
void cpu_interrupt_initialize(void);

register_t cpu_interrupt_disable(void);
void cpu_interrupt_enable(void);
void cpu_interrupt_restore(register_t);

#endif /* !_CPU_INTERRUPT_H_ */

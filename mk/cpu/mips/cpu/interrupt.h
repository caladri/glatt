#ifndef	_CPU_INTERRUPT_H_
#define	_CPU_INTERRUPT_H_

#include <core/queue.h>
#include <cpu/register.h>

#define	CPU_INTERRUPT_COUNT	(8)

typedef	void (interrupt_t)(void *, int);

struct interrupt_handler {
	interrupt_t *ih_func;
	void *ih_arg;
	STAILQ_ENTRY(struct interrupt_handler) ih_link;
};

void cpu_interrupt_establish(int, interrupt_t *, void *) __non_null(2);

void cpu_interrupt(void);
void cpu_interrupt_setup(void);

register_t cpu_interrupt_disable(void);
void cpu_interrupt_restore(register_t);

#endif /* !_CPU_INTERRUPT_H_ */

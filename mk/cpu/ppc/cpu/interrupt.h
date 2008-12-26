#ifndef	_CPU_INTERRUPT_H_
#define	_CPU_INTERRUPT_H_

#include <cpu/register.h>

register_t cpu_interrupt_disable(void);
void cpu_interrupt_restore(register_t);

#endif /* !_CPU_INTERRUPT_H_ */

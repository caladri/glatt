#ifndef	_CPU_INTERRUPT_H_
#define	_CPU_INTERRUPT_H_

register_t cpu_interrupt_disable(void);
void cpu_interrupt_enable(void);
void cpu_interrupt_restore(register_t);

#endif /* !_CPU_INTERRUPT_H_ */

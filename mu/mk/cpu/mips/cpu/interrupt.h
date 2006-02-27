#ifndef	_CPU_INTERRUPT_H_
#define	_CPU_INTERRUPT_H_

#define	CPU_HARD_INTERRUPT_MAX	(6)
#define	CPU_SOFT_INTERRUPT_MAX	(2)

typedef	void (interrupt_t)(void *, int);

struct interrupt_handler {
	interrupt_t *ih_func;
	void *ih_arg;
};

void cpu_hard_interrupt_establish(int, interrupt_t *, void *);
void cpu_soft_interrupt_establish(int, interrupt_t *, void *);

void cpu_interrupt(void);
void cpu_interrupt_initialize(void);

register_t cpu_interrupt_disable(void);
void cpu_interrupt_enable(void);
void cpu_interrupt_restore(register_t);

#endif /* !_CPU_INTERRUPT_H_ */

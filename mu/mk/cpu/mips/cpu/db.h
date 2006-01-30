#ifndef	_CPU_DB_H_
#define	_CPU_DB_H_

static inline void __attribute__ ((__noreturn__))
cpu_break(void)
{
	asm volatile ("break 7" : : : "memory");
	for (;;) continue;
}

#endif /* !_CPU_DB_H_ */

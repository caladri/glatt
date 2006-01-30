#ifndef	_CPU_DB_H_
#define	_CPU_DB_H_

static inline void __noreturn
cpu_break(void)
{
	__asm __volatile ("break 7" : : : "memory");
	for (;;) continue;
}

#endif /* !_CPU_DB_H_ */

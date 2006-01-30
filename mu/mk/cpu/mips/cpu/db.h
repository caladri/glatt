#ifndef	_CPU_DB_H_
#define	_CPU_DB_H_

static inline void
cpu_break(void)
{
	asm volatile ("break 7" : : : "memory");
}

#endif /* !_CPU_DB_H_ */

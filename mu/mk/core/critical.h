#ifndef	_CORE_CRITICAL_H_
#define	_CORE_CRITICAL_H_

#include <cpu/critical.h>

static inline critical_section_t
critical_enter(void)
{
	return (cpu_critical_enter());
}

static inline void
critical_exit(critical_section_t crit)
{
	cpu_critical_exit(crit);
}

#endif /* !_CORE_CRITICAL_H_ */

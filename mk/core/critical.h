#ifndef	_CORE_CRITICAL_H_
#define	_CORE_CRITICAL_H_

#include <cpu/critical.h>

static inline void
critical_enter(void)
{
	cpu_critical_enter();
}

static inline void
critical_exit(void)
{
	cpu_critical_exit();
}

static inline bool
critical_section(void)
{
	return (cpu_critical_section());
}

#endif /* !_CORE_CRITICAL_H_ */

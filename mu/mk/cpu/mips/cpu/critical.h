#ifndef	_CPU_CRITICAL_H_
#define	_CPU_CRITICAL_H_

#include <cpu/register.h>

typedef	register_t	critical_section_t;

critical_section_t cpu_critical_enter(void);
void cpu_critical_exit(critical_section_t);
bool cpu_critical_section(void);

#endif /* !_CPU_CRITICAL_H_ */

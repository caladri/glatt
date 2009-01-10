#ifndef	_CPU_CRITICAL_H_
#define	_CPU_CRITICAL_H_

void cpu_critical_enter(void);
void cpu_critical_exit(void);
bool cpu_critical_section(void);

#endif /* !_CPU_CRITICAL_H_ */

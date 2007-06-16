#ifndef	_CPU_STARTUP_H_
#define	_CPU_STARTUP_H_

#include <platform/startup.h>

void cpu_halt(void) __noreturn;
void cpu_startup(void);
void cpu_startup_thread(void *);

#endif /* !_CPU_STARTUP_H_ */

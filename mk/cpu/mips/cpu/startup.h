#ifndef	_CPU_STARTUP_H_
#define	_CPU_STARTUP_H_

#include <platform/startup.h>

struct thread;

void cpu_break(void) __noreturn;
void cpu_halt(void) __noreturn;
void cpu_startup(paddr_t);
void cpu_startup_thread(struct thread *, void *);
void cpu_wait(void);

#endif /* !_CPU_STARTUP_H_ */

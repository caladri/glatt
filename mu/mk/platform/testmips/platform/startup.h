#ifndef _PLATFORM_STARTUP_H_
#define	_PLATFORM_STARTUP_H_

void platform_console_init(void);
void platform_halt(void) __noreturn;
void platform_start(void);
void platform_startup(void);

#endif /* !_PLATFORM_STARTUP_H_ */

#ifndef _PLATFORM_STARTUP_H_
#define	_PLATFORM_STARTUP_H_

void testmips_console_init(void); /* XXX */

void platform_halt(void) __noreturn;
void platform_start(void);
void platform_startup(void);

#endif /* !_PLATFORM_STARTUP_H_ */

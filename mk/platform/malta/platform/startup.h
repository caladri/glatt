#ifndef	_PLATFORM_STARTUP_H_
#define	_PLATFORM_STARTUP_H_

void platform_console_init(void);
void platform_halt(void) __noreturn;
void platform_start(int32_t, int32_t, int32_t, uint32_t);
void platform_startup(void);
void platform_startup_thread(void);

#endif /* !_PLATFORM_STARTUP_H_ */

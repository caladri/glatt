#ifndef _PLATFORM_STARTUP_H_
#define	_PLATFORM_STARTUP_H_

void platform_halt(void) __noreturn;
void platform_start(int32_t, int32_t, int32_t, uint32_t);
void platform_startup(void);

#endif /* !_PLATFORM_STARTUP_H_ */

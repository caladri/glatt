#ifndef _PLATFORM_STARTUP_H_
#define	_PLATFORM_STARTUP_H_

void platform_halt(void) __noreturn;
void platform_ofw_init(uintptr_t);
vaddr_t platform_start(uintptr_t, uint32_t, uintptr_t, uintptr_t, uint32_t);
void platform_startup(void);
void platform_startup_thread(void);

#endif /* !_PLATFORM_STARTUP_H_ */

#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <platform/clock.h>

#define	TEST_RTC_DEV_BASE	(0x15000000)

#define	TEST_RTC_DEV_TRIGGER	(0x0000)
#define	TEST_RTC_DEV_USECONDS	(0x0020)

#define	TEST_RTC_DEV_FUNCTION(f)					\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_RTC_DEV_BASE + (f))
#define	TEST_RTC_DEV_READ(f)						\
	(volatile uint64_t)*TEST_RTC_DEV_FUNCTION(f)
#define	TEST_RTC_DEV_WRITE(f, v)					\
	*TEST_RTC_DEV_FUNCTION(f) = (v)

#define	CLOCK_CALIBRATION_RUNS	(10)

/*
 * Calibrate R4K clock subsystem -- return the number of cycles in 1/hz seconds.
 */
unsigned
platform_clock_calibrate(unsigned hz)
{
	return (100000000 / hz);
}

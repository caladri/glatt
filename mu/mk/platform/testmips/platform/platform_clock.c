#include <core/types.h>
#include <core/error.h>
#include <core/spinlock.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <platform/clock.h>

#include <io/console/console.h>

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

static struct spinlock platform_clock_calibrate_lock =
	SPINLOCK_INIT("testmips clock calibration");

static unsigned platform_clock_msecond(void);

/*
 * Calibrate R4K clock subsystem -- return the number of cycles in 1/hz seconds.
 */
unsigned
platform_clock_calibrate(unsigned hz)
{
	uint64_t sum;
	unsigned run;

	sum = 0;

	spinlock_lock(&platform_clock_calibrate_lock);
	for (run = 0; run < CLOCK_CALIBRATION_RUNS; run++) {
		unsigned count[3], ms[3];

		/*
		 * XXX
		 * Switch to useconds.
		 */
restart:	count[0] = cpu_read_count();
		ms[0] = platform_clock_msecond();

		for (;;) {
			count[1] = cpu_read_count();
			if ((ms[1] = platform_clock_msecond()) != ms[0])
				break;
		}

		for (;;) {
			count[2] = cpu_read_count();
			if ((ms[2] = platform_clock_msecond()) != ms[1])
				break;
		}

		/*
		 * If we have more than a millisecond interval between samples,
		 * start over.
		 */
		if (ms[1] != ms[0] + 1 || ms[2] != ms[1] + 1)
			goto restart;

		/*
		 * If the clock wrapped between any samples, start over.
		 */
		if (count[1] < count[0] || count[2] < count[1])
			goto restart;

		/*
		 * The difference between the last two samples should cover one
		 * entire second.
		 */
		sum += count[2] - count[1];
	}
	spinlock_unlock(&platform_clock_calibrate_lock);

	/*
	 * Take mean and convert to number of cycles in 1/hz seconds.
	 */
	return (((sum / CLOCK_CALIBRATION_RUNS) * 1000) / hz);
}

static unsigned
platform_clock_msecond(void)
{
	TEST_RTC_DEV_WRITE(TEST_RTC_DEV_TRIGGER, 0);
	return (TEST_RTC_DEV_READ(TEST_RTC_DEV_USECONDS) / 1000);
}

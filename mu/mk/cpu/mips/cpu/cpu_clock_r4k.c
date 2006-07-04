#include <core/types.h>
#include <core/error.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

#define	CLOCK_INTERRUPT	(7)

#define	CLOCK_CYCLES_PER_HZ	(100000)

static void
clock_interrupt(void *arg, int interrupt)
{
	struct device *device;

	ASSERT(interrupt == CLOCK_INTERRUPT, "stray interrupt");

	device = arg;
	ASSERT(device->d_parent->d_unit == mp_whoami(), "on wrong CPU");

	kcprintf("%s%u: interrupt!\n", device->d_driver->d_name,
		 device->d_unit);
	cpu_write_compare(cpu_read_count() + CLOCK_CYCLES_PER_HZ);
}

static int
clock_probe(struct device *device)
{
	int error;

	ASSERT(device->d_parent->d_unit == mp_whoami(), "on wrong CPU");

	cpu_interrupt_establish(CLOCK_INTERRUPT, clock_interrupt, device);
	cpu_write_compare(cpu_read_count() + CLOCK_CYCLES_PER_HZ);

	return (0);
}

DRIVER(clock_r4k, NULL, clock_probe);
DRIVER_ATTACHMENT(clock_r4k, "cpu");

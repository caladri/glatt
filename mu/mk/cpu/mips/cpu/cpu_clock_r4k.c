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
	/*
	 * XXX
	 * Preemptable threads?
	 */
#if 0
	device_printf(device, "interrupt!");
#endif
	cpu_write_compare(cpu_read_count() + CLOCK_CYCLES_PER_HZ);
}

static int
clock_probe(struct device *device)
{
	ASSERT(device->d_unit == -1, "Must not have a unit number.");
	device->d_unit = device->d_parent->d_unit;

	return (0);
}

static int
clock_attach(struct device *device)
{
	device_printf(device, "%u cycles/hz", CLOCK_CYCLES_PER_HZ);
	cpu_interrupt_establish(CLOCK_INTERRUPT, clock_interrupt, device);
	cpu_write_compare(cpu_read_count() + CLOCK_CYCLES_PER_HZ);

	return (0);
}

DRIVER(clock_r4k, "MIPS R4000-style clock", NULL, DRIVER_FLAG_DEFAULT | DRIVER_FLAG_PROBE_UNIT, clock_probe, clock_attach);
DRIVER_ATTACHMENT(clock_r4k, "cpu");

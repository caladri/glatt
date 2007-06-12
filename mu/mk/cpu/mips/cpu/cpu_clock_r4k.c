#include <core/types.h>
#include <core/clock.h>
#include <core/error.h>
#include <core/malloc.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

#define	CLOCK_INTERRUPT	(7)

#define	CLOCK_CYCLES_PER_HZ	(100000)

struct clock_r4k_softc {
	unsigned csc_cycles_per_hz;
	unsigned csc_last_count;
};

static void
clock_r4k_interrupt(void *arg, int interrupt)
{
	struct device *device;
	struct clock_r4k_softc *csc;
	unsigned count;

	ASSERT(interrupt == CLOCK_INTERRUPT, "stray interrupt");

	device = arg;
	ASSERT(device->d_parent->d_unit == mp_whoami(), "on wrong CPU");
	csc = device->d_softc;

	/*
	 * XXX
	 * Preempt?
	 */

	count = cpu_read_count();
	if (count < csc->csc_last_count)
		panic("%s: need to implement clock wrapping.", __func__);

	cpu_write_compare(count + csc->csc_cycles_per_hz);
	clock_ticks((count - csc->csc_last_count) / csc->csc_cycles_per_hz);
}

static int
clock_r4k_probe(struct device *device)
{
	ASSERT(device->d_unit == -1, "Must not have a unit number.");
	device->d_unit = device->d_parent->d_unit;

	return (0);
}

static int
clock_r4k_attach(struct device *device)
{
	struct clock_r4k_softc *csc;

	csc = malloc(sizeof *csc);
	csc->csc_cycles_per_hz = 100000;
	csc->csc_last_count = cpu_read_count();

	device->d_softc = csc;

	device_printf(device, "%u cycles/hz", csc->csc_cycles_per_hz);
	cpu_interrupt_establish(CLOCK_INTERRUPT, clock_r4k_interrupt, device);
	cpu_write_compare(csc->csc_last_count + csc->csc_cycles_per_hz);

	return (0);
}

DRIVER(clock_r4k, "MIPS R4000-style clock", NULL, DRIVER_FLAG_DEFAULT | DRIVER_FLAG_PROBE_UNIT, clock_r4k_probe, clock_r4k_attach);
DRIVER_ATTACHMENT(clock_r4k, "cpu");

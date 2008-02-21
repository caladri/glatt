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
#include <platform/clock.h>

#define	CLOCK_HZ	(100)
#define	CLOCK_INTERRUPT	(7)

struct clock_r4k_softc {
	unsigned csc_cycles_per_hz;
	unsigned csc_last_count;
};

static void
clock_r4k_interrupt(void *arg, int interrupt)
{
	struct device *device;
	struct clock_r4k_softc *csc;
	unsigned count, cycles;

	ASSERT(interrupt == CLOCK_INTERRUPT, "stray interrupt");

	device = arg;
	ASSERT(device->d_parent->d_unit == mp_whoami(), "on wrong CPU");
	csc = device->d_softc;

	count = cpu_read_count();
	if (count < csc->csc_last_count) {
		unsigned clock_max = ~0;

		cycles = (clock_max - csc->csc_last_count) + count;
	} else {
		cycles = count - csc->csc_last_count;
	}

	cpu_write_compare(count + csc->csc_cycles_per_hz);
	clock_ticks(cycles / csc->csc_cycles_per_hz);
	csc->csc_last_count = count;
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
	unsigned cycles;

	cycles = platform_clock_calibrate(CLOCK_HZ);
	if (cycles == 0)
		return (ERROR_NOT_IMPLEMENTED);

	csc = malloc(sizeof *csc);
	csc->csc_cycles_per_hz = cycles;
	csc->csc_last_count = cpu_read_count();

	device->d_softc = csc;

	device_printf(device, "%u cycles/second (running at %uhz)",
		      csc->csc_cycles_per_hz * CLOCK_HZ, CLOCK_HZ);
	cpu_interrupt_establish(CLOCK_INTERRUPT, clock_r4k_interrupt, device);
	cpu_write_compare(csc->csc_last_count + csc->csc_cycles_per_hz);

	return (0);
}

DRIVER(clock_r4k, "MIPS R4000-style clock", NULL, DRIVER_FLAG_DEFAULT | DRIVER_FLAG_PROBE_UNIT, clock_r4k_probe, clock_r4k_attach);
DRIVER_ATTACHMENT(clock_r4k, "cpu");

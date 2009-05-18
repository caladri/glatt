#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <cpu/cpu.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <io/bus/bus.h>
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
	struct bus_instance *bi;
	struct clock_r4k_softc *csc;
	unsigned count, cycles;

	ASSERT(interrupt == CLOCK_INTERRUPT, "stray interrupt");

	bi = arg;
	csc = bus_softc(bi);

	count = cpu_read_count();
	if (count < csc->csc_last_count) {
		unsigned clock_max = ~0;

		cycles = (clock_max - csc->csc_last_count) + count;
	} else {
		cycles = count - csc->csc_last_count;
	}

	cpu_write_compare(count + csc->csc_cycles_per_hz);
	/* XXX clock_ticks(cycles / csc->csc_cycles_per_hz); */
	csc->csc_last_count = count;
}

static int
clock_r4k_setup(struct bus_instance *bi)
{
	struct clock_r4k_softc *csc;
	unsigned cycles;

	cycles = platform_clock_calibrate(CLOCK_HZ);
	if (cycles == 0)
		return (ERROR_NOT_IMPLEMENTED);

	csc = bus_softc_allocate(bi, sizeof *csc);
	csc->csc_cycles_per_hz = cycles;
	csc->csc_last_count = cpu_read_count();

	cpu_interrupt_establish(CLOCK_INTERRUPT, clock_r4k_interrupt, bi);
	cpu_write_compare(csc->csc_last_count + csc->csc_cycles_per_hz);

	bus_set_description(bi, "%u cycles/second (running at %uhz)",
			    csc->csc_cycles_per_hz * CLOCK_HZ, CLOCK_HZ);

	return (0);
}

BUS_INTERFACE(clockif) {
	.bus_setup = clock_r4k_setup,
};
BUS_ATTACHMENT(clock_r4k, "cpu", clockif);

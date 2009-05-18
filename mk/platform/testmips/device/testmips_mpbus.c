#include <core/types.h>
#include <core/error.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/pcpu.h>
#include <io/bus/bus.h>

static int
mpbus_setup(struct bus_instance *bi)
{
	if ((PCPU_GET(flags) & PCPU_FLAG_BOOTSTRAP) == 0) {
		panic("%s: shouldn't be enumerating on application processor.",
		      __func__);
	}
	return (0);
}

BUS_INTERFACE(mpbusif) {
	.bus_setup = mpbus_setup,
};
BUS_ATTACHMENT(mpbus, "mp", mpbusif);

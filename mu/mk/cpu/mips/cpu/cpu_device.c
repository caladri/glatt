#include <core/types.h>
#include <cpu/cpuinfo.h>
#include <cpu/pcpu.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

static int
cpu_probe(struct device *device)
{
	struct pcpu *pcpu;

	pcpu = PCPU_GET(physaddr);
	device->d_softc = pcpu;
	/*
	 * XXX
	 * Make d_desc storage for the string and copy the driver's string in
	 * and have a device_set_description which can fill it using a printf
	 * format.
	 */
	device->d_desc = PCPU_GET(cpuinfo).cpu_type;
	ASSERT(device->d_unit == mp_whoami(), "unit must match cpu id");

	return (0);
}

DRIVER(cpu, "MIPS CPU", NULL, cpu_probe);

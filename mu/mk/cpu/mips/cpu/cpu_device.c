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
	ASSERT(device->d_unit == mp_whoami(), "unit must match cpu id");

	return (0);
}

DRIVER(cpu, NULL, cpu_probe);

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
	kcprintf("cpu%u: attached to %s.%p\n", mp_whoami(), device->d_parent->d_driver->d_name, (void *)device->d_parent);

	return (0);
}

DRIVER(cpu, NULL, cpu_probe);

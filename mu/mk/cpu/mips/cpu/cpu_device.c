#include <core/types.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>

DB_SHOW_TREE(cpu, cpu, true);

static int
cpu_probe(struct device *device)
{
	ASSERT(device->d_unit == -1, "Must not have a unit number.");
	device->d_unit = mp_whoami();

	/*
	 * XXX
	 * Make d_desc storage for the string and copy the driver's string in
	 * and have a device_set_description which can fill it using a printf
	 * format.
	 */
	device->d_desc = PCPU_GET(cpuinfo).cpu_type;

	return (0);
}

static int
cpu_attach(struct device *device)
{
	struct pcpu *pcpu;

	pcpu = PCPU_GET(physaddr);
	device->d_softc = pcpu;

	device_printf(device, "%s %s revision %u.%u, %d TLB entries. (%s)",
		      PCPU_GET(cpuinfo).cpu_company,
		      PCPU_GET(cpuinfo).cpu_type,
		      (unsigned)PCPU_GET(cpuinfo).cpu_revision_major,
		      (unsigned)PCPU_GET(cpuinfo).cpu_revision_minor,
		      (unsigned)PCPU_GET(cpuinfo).cpu_ntlbs,
		      ((PCPU_GET(flags) & PCPU_FLAG_BOOTSTRAP) == 0 ?
		       "application processor" : "bootstrap processor"));

	cpu_interrupt_setup();

	return (0);
}

DRIVER(cpu, "MIPS CPU", NULL, DRIVER_FLAG_DEFAULT | DRIVER_FLAG_PROBE_UNIT, cpu_probe, cpu_attach);

#include <core/types.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/pcpu.h>
#include <io/device/bus.h>
#include <io/device/device.h>

static void
cpu_describe(struct bus_instance *bi)
{
	bus_printf(bi, "%s %s revision %u.%u, %d TLB entries (%s)",
		   PCPU_GET(cpuinfo).cpu_company, PCPU_GET(cpuinfo).cpu_type,
		   (unsigned)PCPU_GET(cpuinfo).cpu_revision_major,
		   (unsigned)PCPU_GET(cpuinfo).cpu_revision_minor,
		   (unsigned)PCPU_GET(cpuinfo).cpu_ntlbs,
		   ((PCPU_GET(flags) & PCPU_FLAG_BOOTSTRAP) == 0 ?
		    "application processor" : "bootstrap processor"));
}

static int
cpu_enumerate_children(struct bus_instance *bi)
{
	int error;

	error = device_enumerate(bi, "clock_r4k", NULL/*XXX*/);
	if (error != 0)
		return (error);
	return (bus_enumerate_children(bi));
}

static int
cpu_setup(struct bus_instance *bi)
{
	cpu_interrupt_setup();

	return (0);
}

BUS_INTERFACE(cpuif) {
	.bus_describe = cpu_describe,
	.bus_enumerate_children = cpu_enumerate_children,
	.bus_setup = cpu_setup,
};
BUS_ATTACHMENT(cpu, NULL, cpuif);
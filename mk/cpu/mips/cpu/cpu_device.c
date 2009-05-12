#include <core/types.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/pcpu.h>
#include <io/device/bus.h>

static int
cpu_setup(struct bus_instance *bi)
{
	bus_set_description(bi, "%s %s revision %u.%u, %d TLB entries (%s)",
			    PCPU_GET(cpuinfo).cpu_company,
			    PCPU_GET(cpuinfo).cpu_type,
			    (unsigned)PCPU_GET(cpuinfo).cpu_revision_major,
			    (unsigned)PCPU_GET(cpuinfo).cpu_revision_minor,
			    (unsigned)PCPU_GET(cpuinfo).cpu_ntlbs,
			    ((PCPU_GET(flags) & PCPU_FLAG_BOOTSTRAP) == 0 ?
			     "application processor" : "bootstrap processor"));

	return (0);
}

BUS_INTERFACE(cpuif) {
	.bus_setup = cpu_setup,
};
BUS_ATTACHMENT(cpu, NULL, cpuif);

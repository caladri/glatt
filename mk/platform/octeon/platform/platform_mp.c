#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/cpu.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/startup.h>
#include <cpu/tlb.h>
#include <io/bus/bus.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

#ifndef	UNIPROCESSOR
void
platform_mp_ipi_send(cpu_id_t cpu, enum ipi_type ipi)
{
	if (cpu == mp_whoami())
		mp_ipi_receive(ipi);
}

void
platform_mp_ipi_send_but(cpu_id_t cpu, enum ipi_type ipi)
{
	if (cpu != mp_whoami())
		mp_ipi_receive(ipi);
}
#endif

#ifndef	UNIPROCESSOR
unsigned
platform_mp_ncpus(void)
{
	return (1);
}
#endif

void
platform_mp_startup(void)
{
}

cpu_id_t
platform_mp_whoami(void)
{
	return (0);
}

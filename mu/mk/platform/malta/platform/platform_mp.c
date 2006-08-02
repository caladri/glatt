#include <core/types.h>
#include <core/mp.h>
#include <db/db.h>

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

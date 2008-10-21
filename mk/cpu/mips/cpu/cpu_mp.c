#include <core/types.h>
#include <core/mp.h>

void
cpu_mp_ipi_send(cpu_id_t cpu, enum ipi_type ipi)
{
	platform_mp_ipi_send(cpu, ipi);
}

void
cpu_mp_ipi_send_but(cpu_id_t cpu, enum ipi_type ipi)
{
	platform_mp_ipi_send_but(cpu, ipi);
}

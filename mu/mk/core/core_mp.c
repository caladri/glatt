#include <core/types.h>
#include <core/mp.h>
#include <io/device/console/console.h>

void
mp_ipi_send(cpu_id_t cpu, enum ipi_type ipi)
{
	cpu_mp_ipi_send(cpu, ipi);
}

void
mp_ipi_send_all(enum ipi_type ipi)
{
	mp_ipi_send_but(mp_whoami(), ipi);
	mp_ipi_send(mp_whoami(), ipi);
}

void
mp_ipi_send_but(cpu_id_t cpu, enum ipi_type ipi)
{
	cpu_mp_ipi_send_but(cpu, ipi);
}

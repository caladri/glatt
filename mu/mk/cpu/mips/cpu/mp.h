#ifndef	_CPU_MP_H_
#define	_CPU_MP_H_

#include <platform/mp.h>

static inline cpu_id_t
cpu_mp_whoami(void)
{
	return (platform_mp_whoami());
}

void cpu_mp_ipi_send(cpu_id_t, enum ipi_type);
void cpu_mp_ipi_send_but(cpu_id_t, enum ipi_type);

#endif /* !_CPU_MP_H_ */

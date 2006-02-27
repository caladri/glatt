#ifndef	_CORE_MP_H_
#define	_CORE_MP_H_

enum ipi_type {
	IPI_STOP,
};

#include <cpu/mp.h>

static inline cpu_id_t
mp_whoami(void)
{
	return (cpu_mp_whoami());
}

void mp_ipi_send(cpu_id_t, enum ipi_type);
void mp_ipi_send_all(enum ipi_type);
void mp_ipi_send_but(cpu_id_t, enum ipi_type);

#endif /* !_CORE_MP_H_ */

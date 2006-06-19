#ifndef	_CORE_MP_H_
#define	_CORE_MP_H_

#include <cpu/mp.h>

/*
 * XXX
 * Once a CPU is up and running, should use its PCPU data as a much faster
 * whoami.
 */
static inline cpu_id_t
mp_whoami(void)
{
	return (cpu_mp_whoami());
}

typedef	void (mp_ipi_handler_t)(void *, enum ipi_type);

void mp_ipi_receive(enum ipi_type);
void mp_ipi_register(enum ipi_type, mp_ipi_handler_t, void *);
void mp_ipi_send(cpu_id_t, enum ipi_type);
void mp_ipi_send_all(enum ipi_type);
void mp_ipi_send_but(cpu_id_t, enum ipi_type);

#endif /* !_CORE_MP_H_ */

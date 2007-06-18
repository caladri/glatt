#ifndef	_CORE_MP_H_
#define	_CORE_MP_H_

#include <cpu/mp.h>

static inline unsigned
mp_ncpus(void)
{
	return (cpu_mp_ncpus());
}

typedef	void (mp_ipi_handler_t)(void *, enum ipi_type);

cpu_id_t mp_whoami(void);

void mp_cpu_present(cpu_id_t);
void mp_cpu_running(cpu_id_t);
cpu_bitmask_t mp_cpu_running_mask(void);
void mp_cpu_stopped(cpu_id_t);

void mp_hokusai_master(void (*)(void *), void *, void (*)(void *), void *);
void mp_hokusai_synchronize(void (*)(void *), void *);

void mp_ipi_receive(enum ipi_type);
void mp_ipi_register(enum ipi_type, mp_ipi_handler_t, void *);
void mp_ipi_send(cpu_id_t, enum ipi_type);
void mp_ipi_send_all(enum ipi_type);
void mp_ipi_send_but(cpu_id_t, enum ipi_type);

#endif /* !_CORE_MP_H_ */

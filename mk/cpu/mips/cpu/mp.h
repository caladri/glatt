#ifndef	_CPU_MP_H_
#define	_CPU_MP_H_

#include <platform/mp.h>

#ifndef	UNIPROCESSOR
static inline unsigned
cpu_mp_ncpus(void)
{
	return (platform_mp_ncpus());
}

static inline cpu_id_t
cpu_mp_whoami(void)
{
	return (platform_mp_whoami());
}

static inline void
cpu_mp_ipi_send(cpu_id_t cpu, enum ipi_type type)
{
	platform_mp_ipi_send(cpu, type);
}

static inline void
cpu_mp_ipi_send_but(cpu_id_t cpu, enum ipi_type type)
{
	platform_mp_ipi_send_but(cpu, type);
}
#endif

#endif /* !_CPU_MP_H_ */

#ifndef	_CORE_MP_H_
#define	_CORE_MP_H_

#include <cpu/mp.h>

static inline unsigned __check_result
mp_ncpus(void)
{
#ifndef	UNIPROCESSOR
	return (cpu_mp_ncpus());
#else
	return (1);
#endif
}

/*
 * A UNIPROCESSOR system must not define MAXCPUS.  A !UNIPROCESSOR system must.
 */
#if defined(UNIPROCESSOR)
#if defined(MAXCPUS)
#error "Uniprocessor systems must not define MAXCPUS."
#endif
#define	MAXCPUS	1
#else /* !defined(UNIPROCESSOR) */
#if !defined(MAXCPUS)
#error "Multiprocessor systems must define MAXCPUS."
#endif
#endif

#ifndef	UNIPROCESSOR
typedef	void (mp_ipi_handler_t)(void *, enum ipi_type);
#endif

#ifndef	UNIPROCESSOR
cpu_id_t mp_whoami_pcpu(void);
cpu_id_t (*mp_whoami)(void) __check_result;
#else
#define	mp_whoami()	((cpu_id_t)0)
#define	CPU_ID_INVALID	((cpu_id_t)~0)
#endif

#ifndef	UNIPROCESSOR
void mp_cpu_present(cpu_id_t);
cpu_bitmask_t mp_cpu_present_mask(void) __check_result;
void mp_cpu_running(cpu_id_t);
cpu_bitmask_t mp_cpu_running_mask(void) __check_result;
void mp_cpu_stopped(cpu_id_t);

void mp_hokusai_synchronize(cpu_bitmask_t, void (*)(void *), void *) __non_null(2);

void mp_ipi_receive(enum ipi_type);
void mp_ipi_register(enum ipi_type, mp_ipi_handler_t, void *) __non_null(2);
void mp_ipi_send(cpu_id_t, enum ipi_type);
void mp_ipi_send_all(enum ipi_type);
void mp_ipi_send_but(cpu_id_t, enum ipi_type);
#endif

#endif /* !_CORE_MP_H_ */

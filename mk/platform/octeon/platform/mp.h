#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

#include <cpu/atomic.h>

typedef	uint64_t	cpu_bitmask_t;

#ifndef UNIPROCESSOR
#define	MAXCPUS		(8 * sizeof (cpu_bitmask_t))
#else
#define	MAXCPUS		(1)
#endif

static inline bool
cpu_bitmask_is_set(const volatile cpu_bitmask_t *maskp, cpu_id_t cpu)
{
#ifdef INVARIANTS
	if (cpu >= MAXCPUS)
		panic("%s: cpu%u exceeds system limit of %u CPUs.",
		      __func__, cpu, MAXCPUS);
#endif
	if ((atomic_load64(maskp) & ((cpu_bitmask_t)1 << cpu)) == 0)
		return (false);
	return (true);
}

static inline void
cpu_bitmask_set(volatile cpu_bitmask_t *maskp, cpu_id_t cpu)
{
	ASSERT(cpu < MAXCPUS, "CPU cannot exceed bounds of type.");
#ifdef INVARIANTS
	if (cpu_bitmask_is_set(maskp, cpu))
		panic("%s: cannot set bit twice for cpu%u.", __func__, cpu);
#endif
	atomic_set64(maskp, (cpu_bitmask_t)1 << cpu);
}

static inline void
cpu_bitmask_clear(volatile cpu_bitmask_t *maskp, cpu_id_t cpu)
{
	ASSERT(cpu < MAXCPUS, "CPU cannot exceed bounds of type.");
#ifdef INVARIANTS
	if (!cpu_bitmask_is_set(maskp, cpu))
		panic("%s: cannot clear bit twice for cpu%u.", __func__, cpu);
#endif
	atomic_clear64(maskp, (cpu_bitmask_t)1 << cpu);
}

enum ipi_type {
	IPI_NONE	= 0,
	IPI_STOP	= 1,
	IPI_HOKUSAI	= 2,
	IPI_FIRST	= IPI_STOP,
	IPI_LAST	= IPI_HOKUSAI,
};

#define	CPU_ID_INVALID	((cpu_id_t)~0)

void platform_mp_ipi_send(cpu_id_t, enum ipi_type);
void platform_mp_ipi_send_but(cpu_id_t, enum ipi_type);
unsigned platform_mp_ncpus(void);
void platform_mp_startup(void);
cpu_id_t platform_mp_whoami(void);

#endif /* !_PLATFORM_MP_H_ */

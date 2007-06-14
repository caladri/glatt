#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

typedef	uint64_t	cpu_bitmask_t;

#define	MAXCPUS		(8 * sizeof (cpu_bitmask_t))

enum ipi_type {
	IPI_NONE	= 0,
	IPI_STOP	= 1,
	IPI_TLBS	= 2,
	IPI_FIRST	= IPI_STOP,
	IPI_LAST	= IPI_TLBS,
};

#define	CPU_ID_INVALID	((cpu_id_t)~0)

void platform_mp_ipi_send(cpu_id_t, enum ipi_type);
void platform_mp_ipi_send_but(cpu_id_t, enum ipi_type);
unsigned platform_mp_ncpus(void);
void platform_mp_startup(void);
cpu_id_t platform_mp_whoami(void);

	/*
	 * XXX platform_mp_memory uses mp registers but isn't an mp-related
	 * function from a machine-independent perspective.
	 */
size_t platform_mp_memory(void);

#endif /* !_PLATFORM_MP_H_ */

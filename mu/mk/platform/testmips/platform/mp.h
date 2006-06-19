#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

enum ipi_type {
	IPI_NONE	= 0,
	IPI_STOP	= 1,
	IPI_FIRST	= IPI_STOP,
	IPI_LAST	= IPI_STOP,
};

#define	CPU_ID_INVALID	((cpu_id_t)~0)

void platform_mp_ipi_send(cpu_id_t, enum ipi_type);
void platform_mp_ipi_send_but(cpu_id_t, enum ipi_type);
void platform_mp_startup(void);
cpu_id_t platform_mp_whoami(void);

	/*
	 * XXX platform_mp_memory uses mp registers but isn't an mp-related
	 * function from a machine-independent perspective.
	 */
size_t platform_mp_memory(void);

#endif /* !_PLATFORM_MP_H_ */

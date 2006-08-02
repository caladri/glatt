#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

enum ipi_type {
	IPI_NONE	= 0,
	IPI_STOP	= 1,
	IPI_FIRST	= IPI_STOP,
	IPI_LAST	= IPI_STOP,
};

#define	CPU_ID_INVALID	((cpu_id_t)~0)

static __inline cpu_id_t
platform_mp_whoami(void)
{
	return (0);
}

void platform_mp_ipi_send(cpu_id_t, enum ipi_type);
void platform_mp_ipi_send_but(cpu_id_t, enum ipi_type);

#endif /* !_PLATFORM_MP_H_ */

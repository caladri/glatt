#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

	/* Generic MP functions implemented here.  */

cpu_id_t platform_mp_whoami(void);

	/* Platform-specific MP functions.  */

void platform_mp_ipi_send(cpu_id_t, enum ipi_type);
void platform_mp_ipi_send_but(cpu_id_t, enum ipi_type);
	/*
	 * XXX platform_mp_memory uses mp registers but isn't an mp-related
	 * function from a machine-independent perspective.
	 */
size_t platform_mp_memory(void);
void platform_mp_start_all(void);

#endif /* !_PLATFORM_MP_H_ */

#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

typedef	uint16_t	cpu_id_t;

	/* Generic MP functions implemented here.  */

cpu_id_t platform_mp_whoami(void);

	/* Platform-specific MP functions.  */

void platform_mp_start_all(void);

#endif /* !_PLATFORM_MP_H_ */

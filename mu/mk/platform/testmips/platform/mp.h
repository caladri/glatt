#ifndef	_PLATFORM_MP_H_
#define	_PLATFORM_MP_H_

typedef	uint16_t	cpu_id_t;

	/* Generic MP functions implemented here.  */

cpu_id_t platform_mp_whoami(void);
int platform_mp_block_but_one(cpu_id_t);
int platform_mp_unblock_but_one(cpu_id_t);

	/* Platform-specific MP functions.  */

void platform_mp_start_all(void);

#endif /* !_PLATFORM_MP_H_ */

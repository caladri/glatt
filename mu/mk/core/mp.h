#ifndef	_CORE_MP_H_
#define	_CORE_MP_H_

#include <cpu/mp.h>

static inline cpu_id_t
mp_whoami(void)
{
	return (cpu_mp_whoami());
}

int mp_block_but_one(cpu_id_t);
int mp_unblock_but_one(cpu_id_t);

#endif /* !_CORE_MP_H_ */

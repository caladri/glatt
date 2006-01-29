#ifndef	_CPU_MP_H_
#define	_CPU_MP_H_

#include <platform/mp.h>

static inline cpu_id_t
cpu_mp_whoami(void)
{
	return (platform_mp_whoami());
}

static inline int
cpu_mp_block_but_one(cpu_id_t one)
{
	return (platform_mp_block_but_one(one));
}

static inline int
cpu_mp_unblock_but_one(cpu_id_t one)
{
	return (platform_mp_unblock_but_one(one));
}

#endif /* !_CPU_MP_H_ */

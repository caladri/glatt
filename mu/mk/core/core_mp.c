#include <core/types.h>
#include <core/mp.h>
#include <io/device/console/console.h>

int
mp_block_but_one(cpu_id_t one)
{
	if (one != mp_whoami())
		kcprintf("WARNING: blocking my own CPU.\n");
	cpu_mp_block_but_one(one);
	return (0);
}

int
mp_unblock_but_one(cpu_id_t one)
{
	if (one != mp_whoami())
		kcprintf("WARNING: unblocking all but a CPU other than me.\n");
	cpu_mp_unblock_but_one(one);
	return (0);
}

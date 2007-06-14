#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <io/device/console/console.h>

static cpu_bitmask_t mp_cpu_present_bitmask;
static cpu_bitmask_t mp_cpu_running_bitmask;

void
mp_cpu_present(cpu_id_t cpu)
{
	if (cpu_bitmask_is_set(&mp_cpu_present_bitmask, cpu))
		panic("%s: cpu%u is already present!", __func__, cpu);
	cpu_bitmask_set(&mp_cpu_present_bitmask, cpu);
}

void
mp_cpu_running(cpu_id_t cpu)
{
	if (!cpu_bitmask_is_set(&mp_cpu_present_bitmask, cpu))
		panic("%s: cpu%u is not present!", __func__, cpu);
	if (cpu_bitmask_is_set(&mp_cpu_running_bitmask, cpu))
		panic("%s: cpu%u is already running!", __func__, cpu);
	cpu_bitmask_set(&mp_cpu_running_bitmask, cpu);
}

cpu_bitmask_t
mp_cpu_running_mask(void)
{
	return (mp_cpu_running_bitmask);
}

void
mp_cpu_stopped(cpu_id_t cpu)
{
	if (!cpu_bitmask_is_set(&mp_cpu_present_bitmask, cpu))
		panic("%s: cpu%u is not present!", __func__, cpu);
	if (!cpu_bitmask_is_set(&mp_cpu_running_bitmask, cpu))
		panic("%s: cpu%u is already stopped!", __func__, cpu);
	cpu_bitmask_clear(&mp_cpu_running_bitmask, cpu);
}

#if 0
static void
mp_db_whoami(void)
{
	kcprintf("cpu%u\n", mp_whoami());
}
DB_COMMAND(whoami, mp_db_whoami, "Show which CPU the debugger is on.");
#endif

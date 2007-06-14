#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <io/device/console/console.h>

static cpu_bitmask_t mp_cpu_present_bitmask;
static cpu_bitmask_t mp_cpu_running_bitmask;

static inline bool
mp_cpu_is_set(const cpu_bitmask_t *maskp, cpu_id_t cpu)
{
	if (cpu >= MAXCPUS)
		panic("%s: cpu%u exceeds system limit of %u CPUs.",
		      __func__, cpu, MAXCPUS);
	if ((*maskp & ((cpu_bitmask_t)1 << cpu)) == 0)
		return (false);
	return (true);
}

static inline void
mp_cpu_set(cpu_bitmask_t *maskp, cpu_id_t cpu)
{
	ASSERT(cpu < MAXCPUS, "CPU cannot exceed bounds of type.");
	if (mp_cpu_is_set(maskp, cpu))
		panic("%s: cannot set bit twice for cpu%u.", __func__, cpu);
	*maskp ^= (cpu_bitmask_t)1 << cpu;
}

static inline void
mp_cpu_clear(cpu_bitmask_t *maskp, cpu_id_t cpu)
{
	ASSERT(cpu < MAXCPUS, "CPU cannot exceed bounds of type.");
	if (!mp_cpu_is_set(maskp, cpu))
		panic("%s: cannot clear bit twice for cpu%u.", __func__, cpu);
	*maskp ^= (cpu_bitmask_t)1 << cpu;
}

void
mp_cpu_present(cpu_id_t cpu)
{
	if (mp_cpu_is_set(&mp_cpu_present_bitmask, cpu))
		panic("%s: cpu%u is already present!", __func__, cpu);
	mp_cpu_set(&mp_cpu_present_bitmask, cpu);
}

void
mp_cpu_running(cpu_id_t cpu)
{
	if (!mp_cpu_is_set(&mp_cpu_present_bitmask, cpu))
		panic("%s: cpu%u is not present!", __func__, cpu);
	if (mp_cpu_is_set(&mp_cpu_running_bitmask, cpu))
		panic("%s: cpu%u is already running!", __func__, cpu);
	mp_cpu_set(&mp_cpu_running_bitmask, cpu);
}

void
mp_cpu_stopped(cpu_id_t cpu)
{
	if (!mp_cpu_is_set(&mp_cpu_present_bitmask, cpu))
		panic("%s: cpu%u is not present!", __func__, cpu);
	if (!mp_cpu_is_set(&mp_cpu_running_bitmask, cpu))
		panic("%s: cpu%u is already stopped!", __func__, cpu);
	mp_cpu_clear(&mp_cpu_running_bitmask, cpu);
}

#if 0
static void
mp_db_whoami(void)
{
	kcprintf("cpu%u\n", mp_whoami());
}
DB_COMMAND(whoami, mp_db_whoami, "Show which CPU the debugger is on.");
#endif

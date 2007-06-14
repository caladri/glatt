#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <io/device/console/console.h>

static cpu_bitmask_t mp_cpu_bitmask;

void
mp_cpu_running(cpu_id_t cpu)
{
	if (cpu >= MAXCPUS)
		panic("%s: cannot start cpu%u (system limit is %u CPUs.)",
		      __func__, cpu, MAXCPUS);
	if ((mp_cpu_bitmask & ((cpu_bitmask_t)1 << cpu)) != 0)
		panic("%s: cpu%u is already running!", __func__, cpu);
	mp_cpu_bitmask |= (cpu_bitmask_t)1 << cpu;
}

void
mp_cpu_stopped(cpu_id_t cpu)
{
	ASSERT(cpu < MAXCPUS, "CPU must be within MAXCPUS.");
	if ((mp_cpu_bitmask & ((cpu_bitmask_t)1 << cpu)) == 0)
		panic("%s: cpu%u is already stopped!", __func__, cpu);
	mp_cpu_bitmask &= ~((cpu_bitmask_t)1 << cpu);
}

#if 0
static void
mp_db_whoami(void)
{
	kcprintf("cpu%u\n", mp_whoami());
}
DB_COMMAND(whoami, mp_db_whoami, "Show which CPU the debugger is on.");
#endif

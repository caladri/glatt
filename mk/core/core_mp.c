#include <core/types.h>
#include <core/mp.h>
#include <core/startup.h>
#include <cpu/pcpu.h>

static cpu_id_t mp_whoami_early(void);
cpu_id_t (*mp_whoami)(void) = mp_whoami_early;

cpu_id_t
mp_whoami_pcpu(void)
{
	return (PCPU_GET(cpuid));
}

static cpu_id_t
mp_whoami_early(void)
{
	return (cpu_mp_whoami());
}

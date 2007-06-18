#include <core/types.h>
#include <core/startup.h>
#include <cpu/pcpu.h>

cpu_id_t
mp_whoami(void)
{
	if (startup_early)
		return (cpu_mp_whoami());
	return (PCPU_GET(cpuid));
}

#include <sk/types.h>
#include <supervisor/cpu.h>
#include <supervisor/internal.h>

void
Install(cpu_id_t me)
{
	supervisor_cpu_installed(me);
}

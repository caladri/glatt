#include <core/types.h>
#include <cpu/cpu.h>
#include <io/device/console/console.h>

void
cpu_identify(void)
{
	kcprintf("MIPS: Unknown (%#x)\n", cpu_read_prid());
}

#include <sk/types.h>
#include <sk/page.h>
#include <cpu/sk/memory.h>
#include <cpu/supervisor/memory.h>

uintptr_t
supervisor_cpu_memory_direct_map(paddr_t physaddr)
{
	return (XKPHYS_MAP_RAM(physaddr));
}

#include <core/types.h>
#ifdef DB
#include <cpu/cpu.h>
#endif
#include <cpu/pcpu.h>
#ifdef DB
#include <db/db_command.h>
#include <core/console.h>
#endif

#ifdef DB
static void
db_cpu_dump_pcpu(void)
{
	kcprintf("thread              = %p\n", PCPU_GET(thread));

	kcprintf("cpuid               = %x\n", PCPU_GET(cpuid));
	kcprintf("flags               = %x\n", PCPU_GET(flags));

	kcprintf("interrupt_mask      = %x\n", PCPU_GET(interrupt_mask));
	kcprintf("interrupt_enable    = %x\n", PCPU_GET(interrupt_enable));

	kcprintf("critical_count      = %u\n", PCPU_GET(critical_count));
	kcprintf("critical_section    = %lx\n", PCPU_GET(critical_section));

	kcprintf("asidnext            = %u\n", PCPU_GET(asidnext));
}
DB_COMMAND(pcpu, cpu, db_cpu_dump_pcpu);
#endif

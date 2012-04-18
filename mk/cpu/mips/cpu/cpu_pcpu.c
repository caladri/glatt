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
	printf("thread              = %p\n", PCPU_GET(thread));

	printf("cpuid               = %x\n", PCPU_GET(cpuid));
	printf("flags               = %x\n", PCPU_GET(flags));

	printf("interrupt_mask      = %x\n", PCPU_GET(interrupt_mask));
	printf("interrupt_enable    = %x\n", PCPU_GET(interrupt_enable));

	printf("critical_count      = %u\n", PCPU_GET(critical_count));
	printf("critical_section    = %lx\n", PCPU_GET(critical_section));

	printf("asidnext            = %u\n", PCPU_GET(asidnext));
}
DB_COMMAND(pcpu, cpu, db_cpu_dump_pcpu);
#endif

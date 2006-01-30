#include <core/types.h>
#include <core/string.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/memory.h>
#include <cpu/register.h>
#include <db/db.h>
#include <io/device/console/console.h>

#define	EXCEPTION_SPACE			(0x80)

#define	EXCEPTION_BASE_GENERAL		(XKPHYS_MAP(XKPHYS_UC, 0x80000180))
#define	EXCEPTION_BASE_XTLBMISS		(XKPHYS_MAP(XKPHYS_UC, 0x80000080))

extern char exception_vector[], exception_vector_end[];
extern char xtlb_vector[], xtlb_vector_end[];

static void cpu_exception_vector_install(void *, const char *, const char *);

void
cpu_exception_init(void)
{
	cpu_exception_vector_install(EXCEPTION_BASE_GENERAL, exception_vector,
				     exception_vector_end);
	cpu_exception_vector_install(EXCEPTION_BASE_XTLBMISS, xtlb_vector,
				     xtlb_vector_end);
	cpu_write_status(cpu_read_status() & ~CP0_STATUS_BEV);
}

static void
cpu_exception_vector_install(void *base, const char *start, const char *end)
{
	size_t len;

	len = end - start;
	if (len > EXCEPTION_SPACE)
		panic("exception code too big");
	if (len == EXCEPTION_SPACE)
		kcprintf("exception vector out of space\n");
	else if (len + 8 >= EXCEPTION_SPACE)
		kcprintf("exception vector almost out of space\n");
	memcpy(base, start, len);
}

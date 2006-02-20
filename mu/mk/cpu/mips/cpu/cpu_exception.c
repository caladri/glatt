#include <core/types.h>
#include <core/mp.h>
#include <core/string.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/frame.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>
#include <db/db.h>
#include <io/device/console/console.h>

#define	EXCEPTION_SPACE			(0x80)

#define	EXCEPTION_BASE_GENERAL		(XKPHYS_MAP(XKPHYS_UC, 0x00000180))
#define	EXCEPTION_BASE_XTLBMISS		(XKPHYS_MAP(XKPHYS_UC, 0x00000080))

#define	EXCEPTION_INT			(0x00)
#define	EXCEPTION_TLB_MOD		(0x01)
#define	EXCEPTION_TLB_LOAD		(0x02)
#define	EXCEPTION_TLB_STORE		(0x03)
#define	EXCEPTION_ADDRESS_LOAD		(0x04)
#define	EXCEPTION_ADDRESS_STORE		(0x05)
#define	EXCEPTION_INSTRUCTION_BUS_ERROR	(0x06)
#define	EXCEPTION_DATA_BUS_ERROR	(0x07)
#define	EXCEPTION_SYSCALL		(0x08)
#define	EXCEPTION_BREAKPOINT		(0x09)
#define	EXCEPTION_RESERVED		(0x0a)
#define	EXCEPTION_CP_UNAVAILABLE	(0x0b)
#define	EXCEPTION_OVERFLOW		(0x0c)
#define	EXCEPTION_TRAP			(0x0d)
#define	EXCEPTION_VCEI			(0x0e)
#define	EXCEPTION_FLOATING_POINT	(0x0f)
	//.dword	generic_exception	/* Res (16) */
	//.dword	generic_exception	/* Res (17) */
	//.dword	generic_exception	/* Res (18) */
	//.dword	generic_exception	/* Res (19) */
	//.dword	generic_exception	/* Res (20) */
	//.dword	generic_exception	/* Res (21) */
	//.dword	generic_exception	/* Res (22) */
#define	EXCEPTION_WATCHPOINT		(0x17)
	//.dword	generic_exception	/* Res (24) */
	//.dword	generic_exception	/* Res (25) */
	//.dword	generic_exception	/* Res (26) */
	//.dword	generic_exception	/* Res (27) */
	//.dword	generic_exception	/* Res (28) */
	//.dword	generic_exception	/* Res (29) */
	//.dword	generic_exception	/* Res (30) */
#define	EXCEPTION_VCED			(0x1f)


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

void
exception(void)
{
	unsigned cause;
	unsigned code;

	cause = cpu_read_cause();
	code = (cause & CP0_CAUSE_EXCEPTION) >> CP0_CAUSE_EXCEPTION_SHIFT;

	kcprintf("\n\nFatal trap type %u on CPU %u:\n", code, mp_whoami());
	kcprintf("current task        = %p\n", (void *)task_me());
	kcprintf("cause               = %x\n", cause);
	kcprintf("status              = %x\n",
		 (unsigned)pcpu_me()->pc_frame.f_regs[FRAME_STATUS]);
	kcprintf("pc                  = %p\n",
		 (void *)pcpu_me()->pc_frame.f_regs[FRAME_EPC]);
	for (;;)	continue;
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

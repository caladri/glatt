#include <core/types.h>
#include <core/mp.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/frame.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>
#include <cpu/startup.h>
#ifdef DB
#include <db/db.h>
#include <db/db_command.h>
#endif
#include <io/console/console.h>

#define	EXCEPTION_SPACE			(0x80)

#define	EXCEPTION_BASE_UTLBMISS		(XKPHYS_MAP(XKPHYS_UC, 0x00000000))
#define	EXCEPTION_BASE_GENERAL		(XKPHYS_MAP(XKPHYS_UC, 0x00000180))
#define	EXCEPTION_BASE_XTLBMISS		(XKPHYS_MAP(XKPHYS_UC, 0x00000080))

static const char *cpu_exception_names[] = {
#define	EXCEPTION_INT			(0x00)
	"Interrupt",
#define	EXCEPTION_TLB_MOD		(0x01)
	"TLB Modify",
#define	EXCEPTION_TLB_LOAD		(0x02)
	"TLB Load",
#define	EXCEPTION_TLB_STORE		(0x03)
	"TLB Store",
#define	EXCEPTION_ADDRESS_LOAD		(0x04)
	"Address Load",
#define	EXCEPTION_ADDRESS_STORE		(0x05)
	"Address Store",
#define	EXCEPTION_INSTRUCTION_BUS_ERROR	(0x06)
	"Bus Error (Instruction)",
#define	EXCEPTION_DATA_BUS_ERROR	(0x07)
	"Bus Error (Data)",
#define	EXCEPTION_SYSCALL		(0x08)
	"System Call",
#define	EXCEPTION_BREAKPOINT		(0x09)
	"Breakpoint",
#define	EXCEPTION_RESERVED_0A		(0x0a)
	NULL,
#define	EXCEPTION_CP_UNAVAILABLE	(0x0b)
	"Coprocessor Unavailable",
#define	EXCEPTION_OVERFLOW		(0x0c)
	"Overflow",
#define	EXCEPTION_TRAP			(0x0d)
	"Trap",
#define	EXCEPTION_VCEI			(0x0e)
	"Cache Coherency (Instruction)",
#define	EXCEPTION_FLOATING_POINT	(0x0f)
	"Floating Point",
#define	EXCEPTION_RESERVED_10		(0x10)
	NULL,
#define	EXCEPTION_RESERVED_11		(0x11)
	NULL,
#define	EXCEPTION_RESERVED_12		(0x12)
	NULL,
#define	EXCEPTION_RESERVED_13		(0x13)
	NULL,
#define	EXCEPTION_RESERVED_14		(0x14)
	NULL,
#define	EXCEPTION_RESERVED_15		(0x15)
	NULL,
#define	EXCEPTION_RESERVED_16		(0x16)
	NULL,
#define	EXCEPTION_WATCHPOINT		(0x17)
	"Watchpoint",
#define	EXCEPTION_RESERVED_18		(0x18)
	NULL,
#define	EXCEPTION_RESERVED_19		(0x19)
	NULL,
#define	EXCEPTION_RESERVED_1A		(0x1a)
	NULL,
#define	EXCEPTION_RESERVED_1B		(0x1b)
	NULL,
#define	EXCEPTION_RESERVED_1C		(0x1c)
	NULL,
#define	EXCEPTION_RESERVED_1D		(0x1d)
	NULL,
#define	EXCEPTION_RESERVED_1E		(0x1e)
	NULL,
#define	EXCEPTION_VCED			(0x1f)
	"Cache Coherency (Data)",
};

extern char utlb_vector[], utlb_vector_end[];
extern char exception_vector[], exception_vector_end[];
extern char xtlb_vector[], xtlb_vector_end[];

static void cpu_exception_vector_install(void *, const char *, const char *);
static void cpu_exception_frame_dump(struct frame *);
static void cpu_exception_state_dump(void);

void
cpu_exception_init(void)
{
	cpu_exception_vector_install(EXCEPTION_BASE_UTLBMISS, utlb_vector,
				     utlb_vector_end);
	cpu_exception_vector_install(EXCEPTION_BASE_GENERAL, exception_vector,
				     exception_vector_end);
	cpu_exception_vector_install(EXCEPTION_BASE_XTLBMISS, xtlb_vector,
				     xtlb_vector_end);
	cpu_write_status(cpu_read_status() & ~CP0_STATUS_BEV);
}

void
exception(struct frame *frame)
{
	unsigned cause;
	unsigned code;

	cause = cpu_read_cause();
	code = (cause & CP0_CAUSE_EXCEPTION) >> CP0_CAUSE_EXCEPTION_SHIFT;

	switch (code) {
	case EXCEPTION_INT:
		cpu_interrupt();
		break;
	default:
		goto debugger;
	}

	return;
debugger:
	kcputs("\n\n");
	cpu_exception_frame_dump(frame);
#ifdef DB
	db_enter();
#else
	kcputs("Debugger not present, halting...\n");
	cpu_halt();
#endif
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

static void
cpu_exception_frame_dump(struct frame *fp)
{
	unsigned cause;
	unsigned code;

	/*
	 * XXX Get cause from frame?
	 */
	cause = cpu_read_cause();
	code = (cause & CP0_CAUSE_EXCEPTION) >> CP0_CAUSE_EXCEPTION_SHIFT;

	kcprintf("Fatal trap type %u (%s) on CPU %u:\n", code,
		 cpu_exception_names[code] == NULL ? "Reserved" :
		 cpu_exception_names[code], mp_whoami());
	cpu_exception_state_dump();
	if (fp != NULL) {
		kcprintf("pc                  = %p\n",
			 (void *)fp->f_regs[FRAME_EPC]);
		kcprintf("ra                  = %p\n",
			 (void *)fp->f_regs[FRAME_RA]);
		kcprintf("sp                  = %p\n",
			 (void *)fp->f_regs[FRAME_SP]);
	} else
		kcprintf("[Frame unavailable.]\n");
}

static void
cpu_exception_state_dump(void)
{
	struct thread *td;

	td = current_thread();

	kcprintf("thread              = %p (%s)\n",
		 (void *)td, td == NULL ? "nil" : td->td_name);
	if (td != NULL) {
		kcprintf("task                = %p (%s)\n", (void *)td->td_task,
			 td->td_task == NULL ? "nil" : td->td_task->t_name);
	}
	kcprintf("status              = %x\n", cpu_read_status());
	kcprintf("cause               = %x\n", cpu_read_cause());
	kcprintf("badvaddr            = %p\n", (void *)cpu_read_badvaddr());
}
DB_COMMAND(state, cpu, cpu_exception_state_dump);

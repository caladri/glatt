#include <core/types.h>
#include <core/mp.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/frame.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/register.h>
#include <cpu/startup.h>
#include <cpu/syscall.h>
#ifdef DB
#include <db/db.h>
#include <db/db_command.h>
#endif
#include <io/console/console.h>
#include <vm/vm.h>
#include <vm/vm_fault.h>
#include <vm/vm_page.h>

#define	EXCEPTION_SPACE			(0x80)

#define	EXCEPTION_BASE_UTLBMISS		(XKPHYS_MAP(CCA_UC, 0x00000000))
#define	EXCEPTION_BASE_GENERAL		(XKPHYS_MAP(CCA_UC, 0x00000180))
#define	EXCEPTION_BASE_XTLBMISS		(XKPHYS_MAP(CCA_UC, 0x00000080))
#define	EXCEPTION_BASE_INTERRUPT	(XKPHYS_MAP(CCA_UC, 0x00000200))

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
	cpu_exception_vector_install(EXCEPTION_BASE_INTERRUPT, exception_vector,
				     exception_vector_end);
	cpu_write_status(cpu_read_status() & ~CP0_STATUS_BEV);

	/*
	 * Now install the debugger.
	 */
#ifdef	DB
	db_init();
#endif

	/*
	 * Set up interrupts.
	 */
	cpu_interrupt_setup();
}

void
exception(struct frame *frame)
{
	struct frame *oframe;
	struct thread *td;
	unsigned cause;
	unsigned code;
	vaddr_t vaddr;
	bool handled;
	bool user;
	int error;

	ASSERT(frame != NULL, "exception must have a frame.");

	td = current_thread();
	cause = cpu_read_cause();
	code = (cause & CP0_CAUSE_EXCEPTION) >> CP0_CAUSE_EXCEPTION_SHIFT;
	user = (frame->f_regs[FRAME_STATUS] & CP0_STATUS_U) != 0;

	if (td != NULL) {
		oframe = td->td_cputhread.td_frame;
		td->td_cputhread.td_frame = frame;
	} else {
		oframe = NULL; /* XXX GCC -Wuinitialized.  */
	}

	handled = false;

	switch (code) {
	case EXCEPTION_INT:
		cpu_interrupt();
		handled = true;
		break;
	case EXCEPTION_SYSCALL:
		if (!user) {
			kcprintf("Kernel-originated system call.\n");
			break;
		}
		cpu_syscall(frame);
		handled = true;
		break;
	case EXCEPTION_TLB_LOAD:
	case EXCEPTION_TLB_STORE:
		if (!user) {
			kcprintf("Kernel page fault.\n");
			break;
		}
		vaddr = cpu_read_badvaddr();
		if (vaddr > USER_STACK_BOT && vaddr <= USER_STACK_TOP) {
			error = vm_fault_stack(td, vaddr);
			if (error != 0) {
				kcprintf("%s: vm_fault_stack failed: %m", __func__, error);
				break;
			}
			handled = true;
			break;
		}
		kcprintf("Userland page fault.  Not yet implemented.\n");
		break;
	default:
		kcprintf("Unhandled exception.\n");
		break;
	}

	if (!handled) {
		kcputs("\n\n");
		if (td == NULL)
			cpu_exception_frame_dump(frame);
		else
			cpu_exception_state_dump();
#ifdef DB
		db_enter();
#else
		kcputs("Debugger not present, halting...\n");
		cpu_halt();
#endif
	}

	if (td != NULL) {
		td->td_cputhread.td_frame = oframe;
	}
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
	const char *mode;
	unsigned cause;
	unsigned code;

	if (fp == NULL) {
		kcprintf("[Frame unavailable.]\n");
		return;
	}

	/*
	 * XXX Get cause from frame.
	 */
	cause = cpu_read_cause();
	code = (cause & CP0_CAUSE_EXCEPTION) >> CP0_CAUSE_EXCEPTION_SHIFT;

	if ((fp->f_regs[FRAME_STATUS] & CP0_STATUS_U) == 0)
		mode = "kernel";
	else
		mode = "user";

	kcprintf("Trap type %u (%s) in %s mode on CPU %u:\n", code,
		 cpu_exception_names[code] == NULL ? "Reserved" :
		 cpu_exception_names[code], mode, mp_whoami());
	kcprintf("status              = %x\n",
		 (unsigned)fp->f_regs[FRAME_STATUS]);
	kcprintf("pc                  = %p\n",
		 (void *)fp->f_regs[FRAME_EPC]);
	kcprintf("ra                  = %p\n",
		 (void *)fp->f_regs[FRAME_RA]);
	kcprintf("sp                  = %p\n",
		 (void *)fp->f_regs[FRAME_SP]);
}

static void
cpu_exception_state_dump(void)
{
	struct thread *td;

	td = current_thread();

	if (td != NULL) {
		cpu_exception_frame_dump(td->td_cputhread.td_frame);

		kcprintf("thread              = %p (%s)\n",
			 (void *)td, td->td_name);
		kcprintf("task                = %p (%s)\n", (void *)td->td_task,
			 td->td_task == NULL ? "nil" : td->td_task->t_name);
	} else {
		kcprintf("[Thread unavailable.]\n");
	}
	/* XXX Push into frame?  */
	kcprintf("cause               = %x\n", cpu_read_cause());
	kcprintf("badvaddr            = %p\n", (void *)cpu_read_badvaddr());
}
#ifdef DB
DB_COMMAND(state, cpu, cpu_exception_state_dump);
#endif

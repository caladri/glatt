#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

#include <cpu/cpuinfo.h>
#include <cpu/frame.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>

struct task;
struct vm;

#define	PCPU_VIRTUAL	(XKSEG_BASE)

	/* Per-CPU data.  */
struct pcpu {
	struct frame pc_frame;
	struct task *pc_task;
	struct vm *pc_vm;
	struct cpuinfo pc_cpuinfo;
	struct interrupt_handler pc_hard_interrupt[CPU_HARD_INTERRUPT_COUNT];
	struct interrupt_handler pc_soft_interrupt[CPU_SOFT_INTERRUPT_COUNT];
};

#define	PCPU_PTR()							\
	((struct pcpu *)PCPU_VIRTUAL)

#define	PCPU_GET(field)							\
	(PCPU_PTR()->pc_ ## field)

#define	PCPU_SET(field, value)						\
	(PCPU_PTR()->pc_ ## field = (value))

#endif /* !_CPU_PCPU_H_ */

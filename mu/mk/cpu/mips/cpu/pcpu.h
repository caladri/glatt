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
	struct interrupt_handler pc_hard_interrupt[CPU_HARD_INTERRUPT_MAX];
	struct interrupt_handler pc_soft_interrupt[CPU_SOFT_INTERRUPT_MAX];
};

static __inline struct pcpu *
pcpu_me(void)
{
	/*
	 * XXX
	 * Should be volatile in time.  Or use a PCPU_GET/SET like FreeBSD.
	 */
	return ((struct pcpu *)PCPU_VIRTUAL);
}

static __inline volatile struct task *
task_me(void)
{
	return (pcpu_me()->pc_task);
}

#endif /* !_CPU_PCPU_H_ */

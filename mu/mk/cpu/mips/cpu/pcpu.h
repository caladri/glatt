#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

#include <cpu/cpuinfo.h>
#include <cpu/memory.h>

struct task;

#define	PCPU_VIRTUAL	(XKSEG_BASE)

	/* Per-CPU data.  */
struct pcpu {
	struct task *pc_task;
	struct cpuinfo pc_cpuinfo;
};

static __inline volatile struct pcpu *
pcpu_me(void)
{
	return ((volatile struct pcpu *)PCPU_VIRTUAL);
}

static __inline volatile struct task *
task_me(void)
{
	return (pcpu_me()->pc_task);
}

#endif /* !_CPU_PCPU_H_ */

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
pcpu_get(void)
{
	return ((volatile struct pcpu *)PCPU_VIRTUAL);
}

#endif /* !_CPU_PCPU_H_ */

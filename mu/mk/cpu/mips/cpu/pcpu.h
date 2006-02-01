#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

#include <cpu/cpuinfo.h>
#include <cpu/memory.h>

#define	PCPU_VIRTUAL	(XKSEG_BASE)

	/* Per-CPU data.  */
struct pcpu {
	struct cpuinfo pc_cpuinfo;
};

static __inline struct pcpu *
pcpu_get(void)
{
	return ((struct pcpu *)PCPU_VIRTUAL);
}

#endif /* !_CPU_PCPU_H_ */

#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

#ifndef	ASSEMBLER
#include <cpu/cpuinfo.h>
#include <cpu/frame.h>
#include <cpu/memory.h>
#endif

#ifndef	ASSEMBLER
struct task;
struct vm;

	/* Per-CPU data.  */
struct pcpu {
	struct frame pc_frame;
	struct task *pc_task;
	struct vm *pc_vm;
	struct cpuinfo pc_cpuinfo;
};
#endif

#define	PCPU_VIRTUAL	(XKSEG_BASE)

#ifndef	ASSEMBLER
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
#endif

#endif /* !_CPU_PCPU_H_ */

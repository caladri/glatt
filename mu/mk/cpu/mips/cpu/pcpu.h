#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

#include <core/queue.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>

struct device;
struct ipc_queue;
struct scheduler_queue;
struct thread;

#define	PCPU_VIRTUAL	(KERNEL_BASE)

#define	PCPU_FLAG_RUNNING	(0x00000001)
#define	PCPU_FLAG_PANICKED	(0x00000002)
#define	PCPU_FLAG_BOOTSTRAP	(0x00000004)

	/* Per-CPU data.  */
struct pcpu {
	/* Current running thread.  */
	struct thread *pc_thread;

	/* Information about this CPU (and its device in the tree.)  */
	struct pcpu *pc_physaddr;
	cpu_id_t pc_cpuid;
	struct cpuinfo pc_cpuinfo;
	struct device *pc_device;
	unsigned pc_flags;

	/* Interrupt data.  */
	STAILQ_HEAD(, struct interrupt_handler)
		pc_interrupt_table[CPU_INTERRUPT_COUNT];
	register_t pc_interrupt_mask;
	register_t pc_interrupt_enable;

	/* For ASID allocator in page mapping code.  */
	unsigned pc_asidnext;

	/* Per-CPU data structures for various MI code.  */
	struct scheduler_queue *pc_runq;
	struct ipc_queue *pc_ipc_queue;
};

#define	PCPU_PTR()							\
	((struct pcpu *)PCPU_VIRTUAL)

#define	PCPU_GET(field)							\
	(PCPU_PTR()->pc_ ## field)

#define	PCPU_SET(field, value)						\
	(PCPU_PTR()->pc_ ## field = (value))

#endif /* !_CPU_PCPU_H_ */

#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

#include <core/clock.h> /* XXX clock_ticks_t in types.h? */
#include <core/ipc.h>
#include <core/scheduler.h>
#include <core/queue.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>

struct device;
struct thread;

#define	PCPU_VIRTUAL	(KERNEL_BASE)

#define	PCPU_FLAG_RUNNING	(0x00000001)
#define	PCPU_FLAG_PANICKED	(0x00000002)

	/* Per-CPU data.  */
struct pcpu {
	struct thread *pc_thread;
	struct thread *pc_maintd;
	cpu_id_t pc_cpuid;
	struct cpuinfo pc_cpuinfo;
	struct pcpu *pc_physaddr;
	struct device *pc_device;
	unsigned pc_flags;
	STAILQ_HEAD(, struct interrupt_handler) pc_interrupt_table[CPU_INTERRUPT_COUNT];
	register_t pc_interrupt_mask;
	struct scheduler_queue *pc_scheduler;
	struct ipc_queue *pc_ipc_queue;
	unsigned pc_asidnext;
	clock_ticks_t pc_clock;
	register_t pc_interrupt_enable;
};

#define	PCPU_PTR()							\
	((struct pcpu *)PCPU_VIRTUAL)

#define	PCPU_GET(field)							\
	(PCPU_PTR()->pc_ ## field)

#define	PCPU_SET(field, value)						\
	(PCPU_PTR()->pc_ ## field = (value))

#endif /* !_CPU_PCPU_H_ */

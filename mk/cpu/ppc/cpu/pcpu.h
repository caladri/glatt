#ifndef	_CPU_PCPU_H_
#define	_CPU_PCPU_H_

struct thread;

#define	PCPU_FLAG_RUNNING	(0x00000001)
#define	PCPU_FLAG_PANICKED	(0x00000002)

	/* Per-CPU data.  */
struct pcpu {
	/* Current running thread.  */
	struct thread *pc_thread;

	/* Information about this CPU (and its device in the tree.)  */
	unsigned pc_flags;
};

static inline struct pcpu *
pcpu_get(void)
{
	struct pcpu *pcpu;

	__asm __volatile ("mfsprg %[pcpu], 0" : [pcpu] "=&r"(pcpu));

	return (pcpu);
}

#define	PCPU_PTR()							\
	(pcpu_get())

#define	PCPU_GET(field)							\
	(PCPU_PTR()->pc_ ## field)

#define	PCPU_SET(field, value)						\
	(PCPU_PTR()->pc_ ## field = (value))

#endif /* !_CPU_PCPU_H_ */

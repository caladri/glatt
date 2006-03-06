#ifndef	_CORE_THREAD_H_
#define	_CORE_THREAD_H_

#include <cpu/context.h>
#include <cpu/frame.h>
#include <cpu/pcpu.h>
#include <cpu/thread.h>

#define	THREAD_NAME_SIZE	(128)

#define	THREAD_DEFAULT	(0x00000000)	/* Default thread flags.  */

struct thread {
	struct task *td_parent;
	struct thread *td_next;
	char td_name[THREAD_NAME_SIZE];
	uint32_t td_flags;
	struct frame td_frame;		/* Frame for interrupts and such.  */
	struct context td_context;	/* State for context switching.  */
	void *td_kstack;
};

int thread_create(struct thread **, struct task *, const char *, uint32_t);

#endif /* !_CORE_THREAD_H_ */

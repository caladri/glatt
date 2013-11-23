#ifndef	_CPU_THREAD_H_
#define	_CPU_THREAD_H_

#include <cpu/tlb.h>

struct thread;

#define	KSTACK_SIZE	(2 * PAGE_SIZE)
#define	USTACK_SIZE	(128 * PAGE_SIZE)
#define	MAILBOX_SIZE	(1 * PAGE_SIZE)

#define	current_thread()	PCPU_GET(thread)

struct cpu_thread {
	struct frame *td_frame;
	struct tlb_wired_context td_tlbwired;
	vaddr_t td_mbox;
};

void cpu_thread_activate(struct thread *);
void cpu_thread_free(struct thread *) __non_null(1);
void cpu_thread_set_upcall(struct thread *, void (*)(struct thread *, void *), void *) __non_null(1, 2);
void cpu_thread_set_upcall_user(struct thread *, vaddr_t, register_t) __non_null(1);
int cpu_thread_setup(struct thread *) __non_null(1);
void cpu_thread_trampoline(struct thread *, void (*)(struct thread *, void *), void *) __non_null(1, 2);
void cpu_thread_trampoline_user(struct thread *, vaddr_t, register_t) __non_null(1);

#endif /* !_CPU_THREAD_H_ */

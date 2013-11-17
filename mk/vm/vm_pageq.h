#ifndef	_VM_VM_PAGEQ_H_
#define	_VM_VM_PAGEQ_H_

#include <core/queue.h>

struct vm_page;

struct vm_page_queue {
	TAILQ_HEAD(, struct vm_page) pq_queue;
};

#endif /* !_VM_VM_PAGEQ_H_ */

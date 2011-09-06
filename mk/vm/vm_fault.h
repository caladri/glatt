#ifndef	_VM_VM_FAULT_H_
#define	_VM_VM_FAULT_H_

struct thread;

int vm_fault_stack(struct thread *, vaddr_t);

#endif /* !_VM_VM_FAULT_H_ */

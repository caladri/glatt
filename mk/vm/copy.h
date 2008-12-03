#ifndef	_VM_COPY_H_
#define	_VM_COPY_H_

struct vm;

int vm_copy(struct vm *, vaddr_t, size_t, struct vm *, vaddr_t *);

#endif /* !_VM_COPY_H_ */

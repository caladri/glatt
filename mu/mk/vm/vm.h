#ifndef	_VM_VM_H_
#define	_VM_VM_H_

#include <vm/types.h>

struct pmap;

struct vm {
	struct pmap *vm_pmap;
};

extern struct vm kernel_vm;

int vm_find_address(struct vm *, vaddr_t *);
int vm_free_address(struct vm *, vaddr_t);

#endif /* !_VM_VM_H_ */

#ifndef	_VM_VM_H_
#define	_VM_VM_H_

#include <vm/types.h>

struct pmap;
struct vm_index;

struct vm {
	struct pmap *vm_pmap;
	struct vm_index *vm_index;
};

extern struct vm kernel_vm;

int vm_alloc_address(struct vm *, vaddr_t *, size_t);
int vm_free_address(struct vm *, vaddr_t, size_t);
int vm_insert_range(struct vm *, vaddr_t, vaddr_t);

#endif /* !_VM_VM_H_ */

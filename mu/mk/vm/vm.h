#ifndef	_VM_VM_H_
#define	_VM_VM_H_

#include <core/spinlock.h>
#include <vm/types.h>

struct pmap;
struct vm_index;

struct vm {
	struct spinlock vm_lock;
	struct pmap *vm_pmap;
	struct vm_index *vm_index;
	struct vm_index *vm_index_free;
};

extern struct vm kernel_vm;

void vm_init(void);

int vm_alloc_address(struct vm *, vaddr_t *, size_t);
int vm_free_address(struct vm *, vaddr_t);
int vm_insert_range(struct vm *, vaddr_t, vaddr_t);
int vm_setup(struct vm *);

#endif /* !_VM_VM_H_ */

#ifndef	_VM_VM_H_
#define	_VM_VM_H_

#include <core/queue.h>
#include <core/spinlock.h>
#include <vm/types.h>

struct pmap;
struct vm_index;

struct vm {
	struct spinlock vm_lock;
	struct pmap *vm_pmap;
	struct vm_index *vm_index;
	TAILQ_HEAD(, struct vm_index) vm_index_free;
};
#define	VM_LOCK(vm)	spinlock_lock(&(vm)->vm_lock)
#define	VM_UNLOCK(vm)	spinlock_unlock(&(vm)->vm_lock)

extern struct vm kernel_vm;

void vm_init(void);
int vm_init_index(void);

int vm_alloc_address(struct vm *, vaddr_t *, size_t);
int vm_free_address(struct vm *, vaddr_t);
int vm_insert_range(struct vm *, vaddr_t, vaddr_t);
int vm_setup(struct vm **, vaddr_t, vaddr_t);

#endif /* !_VM_VM_H_ */

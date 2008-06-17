#ifndef	_VM_VM_H_
#define	_VM_VM_H_

#include <core/queue.h>
#include <core/spinlock.h>
#ifdef DB
#include <db/db_show.h>
#endif
#include <vm/types.h>

struct pmap;
struct vm_index;
struct vm_page;

#ifdef DB
DB_SHOW_TREE_DECLARE(vm);
DB_SHOW_TREE_DECLARE(vm_index);
#endif

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

int vm_page_copy(struct vm *, vaddr_t, struct vm *, vaddr_t *);
int vm_setup(struct vm **, vaddr_t, vaddr_t);

#endif /* !_VM_VM_H_ */

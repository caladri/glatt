#ifndef	_VM_VM_H_
#define	_VM_VM_H_

#include <core/btree.h>
#include <core/queue.h>
#include <core/spinlock.h>
#ifdef DB
#include <db/db_command.h>
#endif

struct pmap;
struct vm_index;
struct vm_page;

#ifdef DB
DB_COMMAND_TREE_DECLARE(vm);
DB_COMMAND_TREE_DECLARE(vm_index);
#endif

struct vm {
	struct spinlock vm_lock;
	struct pmap *vm_pmap;
	BTREE_ROOT(struct vm_index) vm_index;
	BTREE_ROOT(struct vm_index) vm_index_free;
};
#define	VM_LOCK(vm)	spinlock_lock(&(vm)->vm_lock)
#define	VM_UNLOCK(vm)	spinlock_unlock(&(vm)->vm_lock)

extern struct vm kernel_vm;

void vm_init(void);

void vm_exit(struct vm *) __non_null(1);
int vm_setup(struct vm **, vaddr_t, vaddr_t) __non_null(1) __check_result;

#endif /* !_VM_VM_H_ */

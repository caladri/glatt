#include <core/types.h>
#include <core/error.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	VM_LOCK(vm)	spinlock_lock(&(vm)->vm_lock)
#define	VM_UNLOCK(vm)	spinlock_unlock(&(vm)->vm_lock)

struct vm kernel_vm;

void
vm_init(void)
{
	int error;

	error = vm_init_index();
	if (error != 0)
		panic("%s: vm_init_index failed: %u", __func__, error);
}

int
vm_setup(struct vm *vm)
{
	spinlock_init(&vm->vm_lock, "VM lock");
	VM_LOCK(vm);
	vm->vm_pmap = NULL;
	vm->vm_index = NULL;
	vm->vm_index_free = NULL;
	VM_UNLOCK(vm);
	return (0);
}

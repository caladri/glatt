#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

struct vm kernel_vm;
static struct pool vm_pool;

void
vm_init(void)
{
	int error;

	/* XXX must initialize kernel_vm like vm_setup VMs.  */
	spinlock_init(&kernel_vm.vm_lock, "Kernel VM lock");

	error = pool_create(&vm_pool, "VM", sizeof (struct vm), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
	error = vm_init_index();
	if (error != 0)
		panic("%s: vm_init_index failed: %m", __func__, error);
}

int
vm_setup(struct vm **vmp, vaddr_t base, vaddr_t end)
{
	struct vm *vm;
	int error;

	vm = pool_allocate(&vm_pool);
	if (vm == NULL)
		return (ERROR_EXHAUSTED);

	spinlock_init(&vm->vm_lock, "VM lock");
	VM_LOCK(vm);
	vm->vm_pmap = NULL;
	error = pmap_init(vm, base, end);
	if (error != 0) {
		VM_UNLOCK(vm);
		pool_free(&vm_pool, vm);
		return (error);
	}
	vm->vm_index = NULL;
	TAILQ_INIT(&vm->vm_index_free);
	VM_UNLOCK(vm);
	return (0);
}

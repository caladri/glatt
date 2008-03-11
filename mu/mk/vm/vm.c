#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

DB_SHOW_TREE(vm, vm, true);

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

	/*
	 * Bring up PMAP.
	 */
	pmap_bootstrap();
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
	vm->vm_index = NULL;
	TAILQ_INIT(&vm->vm_index_free);
	error = pmap_init(vm, base, end);
	if (error != 0) {
		VM_UNLOCK(vm);
		pool_free(vm);
		return (error);
	}
	*vmp = vm;
	VM_UNLOCK(vm);
	return (0);
}

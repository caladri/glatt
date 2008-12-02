#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <cpu/pmap.h>
#ifdef DB
#include <db/db_show.h>
#endif
#include <vm/alloc.h>
#include <vm/index.h>
#include <vm/page.h>
#include <vm/vm.h>

#ifdef DB
DB_SHOW_TREE(vm, vm);
DB_SHOW_VALUE_TREE(vm, root, DB_SHOW_TREE_POINTER(vm));
#endif

struct vm kernel_vm;
static struct pool vm_pool;

void
vm_init(void)
{
	int error;

	/* XXX must initialize kernel_vm like vm_setup VMs.  */
	spinlock_init(&kernel_vm.vm_lock, "Kernel VM lock", SPINLOCK_FLAG_DEFAULT | SPINLOCK_FLAG_RECURSE);

	TAILQ_INIT(&kernel_vm.vm_index_free);

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
vm_page_copy(struct vm *vms, vaddr_t src, struct vm *vmd, vaddr_t *dstp)
{
	vaddr_t dst;
	int error, error2;

	if (dstp != NULL)
		return (ERROR_INVALID);

	if (vms == &kernel_vm || vmd == &kernel_vm)
		return (ERROR_NOT_PERMITTED);

	if (vms == vmd) {
		*dstp = src;
		return (0);
	}

	error = vm_alloc_page(vmd, &dst);
	if (error != 0)
		return (error);

	error = page_copy(vms, src, vmd, dst);
	if (error != 0) {
		error2 = vm_free_page(vmd, dst);
		if (error2 != 0)
			panic("%s: vm_free_page failed: %m", __func__, error2);
		return (error);
	}

	return (0);
}

int
vm_setup(struct vm **vmp, vaddr_t base, vaddr_t end)
{
	struct vm *vm;
	int error;

	vm = pool_allocate(&vm_pool);
	if (vm == NULL)
		return (ERROR_EXHAUSTED);

	spinlock_init(&vm->vm_lock, "VM lock", SPINLOCK_FLAG_DEFAULT | SPINLOCK_FLAG_RECURSE);
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

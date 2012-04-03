#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <cpu/pmap.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <vm/vm.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

#ifdef DB
DB_COMMAND_TREE(vm, root, vm);
#endif

struct vm kernel_vm;

static struct pool vm_pool;

static int vm_setup2(struct vm *, const char *);

void
vm_init(void)
{
	int error;

	/*
	 * Set up kernel_vm.  Must happen before pmap_bootstrap or anything
	 * else that might depend on validity of MI parts of the VM structure.
	 */
	error = vm_setup2(&kernel_vm, "Kernel VM");
	if (error != 0)
		panic("%s: vm_setup2 failed: %m", __func__, error);

	/*
	 * Initialize vm_index module.  Must happen before pmap_bootstrap,
	 * which calls pmap_init, which calls vm_insert_range.
	 */
	error = vm_init_index();
	if (error != 0)
		panic("%s: vm_init_index failed: %m", __func__, error);

	/*
	 * Bring up PMAP.
	 */
	pmap_bootstrap();

	/*
	 * Set up pool to allocate user VM structures.  Must happen before any
	 * tasks are created.
	 */
	error = pool_create(&vm_pool, "VM", sizeof (struct vm), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}

void
vm_exit(struct vm *vm)
{
	/*
	 * XXX
	 * Needs to be implemented.
	 */
}

int
vm_setup(struct vm **vmp, vaddr_t base, vaddr_t end)
{
	struct vm *vm;
	int error;

	vm = pool_allocate(&vm_pool);
	if (vm == NULL)
		return (ERROR_EXHAUSTED);

	error = vm_setup2(vm, "User VM");
	if (error != 0) {
		pool_free(vm);
		return (error);
	}

	VM_LOCK(vm);
	error = pmap_init(vm, base, end);
	if (error != 0) {
		VM_UNLOCK(vm);
		pool_free(vm);
		return (error);
	}
	*vmp = vm;
	VM_UNLOCK(vm);

	error = vm_alloc_range(vm, USER_STACK_BOT, USER_STACK_TOP);
	if (error != 0)
		panic("%s: could not allocate range for stack: %m", __func__, error);
	return (0);
}

static int
vm_setup2(struct vm *vm, const char *name)
{
	spinlock_init(&vm->vm_lock, name,
		      SPINLOCK_FLAG_DEFAULT | SPINLOCK_FLAG_RECURSE);
	vm->vm_pmap = NULL;
	BTREE_ROOT_INIT(&vm->vm_index);
	BTREE_ROOT_INIT(&vm->vm_index_free);

	return (0);
}

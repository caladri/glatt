#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <io/console/console.h>
#include <vm/index.h>
#include <vm/map.h>
#include <vm/page.h>
#include <vm/vm.h>

DB_SHOW_TREE(vm_index, index);
DB_SHOW_VALUE_TREE(index, vm, DB_SHOW_TREE_POINTER(vm_index));

static struct pool vm_index_pool;

static void vm_free_index(struct vm *, struct vm_index *);
static int vm_use_index(struct vm *, struct vm_index *, size_t);

int
vm_init_index(void)
{
	int error;

	error = pool_create(&vm_index_pool, "VM Index",
			    sizeof (struct vm_index), POOL_DEFAULT);
	if (error != 0)
		return (error);
	return (0);
}

int
vm_alloc_address(struct vm *vm, vaddr_t *vaddrp, size_t pages)
{
	struct vm_index *vmi;
	struct vm_index *t;
	int error;

	vmi = NULL;

	VM_LOCK(vm);
	/* Look for a best match.  */
	TAILQ_FOREACH(t, &vm->vm_index_free, vmi_free_link) {
		if (t->vmi_size == pages) {
			error = vm_use_index(vm, t, pages);
			if (error != 0) {
				VM_UNLOCK(vm);
				return (error);
			}
			*vaddrp = t->vmi_base;
			VM_UNLOCK(vm);
			return (0);
		}
		if (t->vmi_size > pages) {
			if (vmi == NULL) {
				vmi = t;
			} else {
				if (vmi->vmi_size > t->vmi_size)
					vmi = t;
			}
		}
	}
	if (vmi == NULL) {
		/* XXX Collapse tree and rebalance if possible.  */
		kcprintf("%s: need to collapse tree.\n", __func__);
		VM_UNLOCK(vm);
		return (ERROR_NOT_IMPLEMENTED);
	}
	error = vm_use_index(vm, vmi, pages);
	if (error != 0) {
		VM_UNLOCK(vm);
		return (error);
	}
	*vaddrp = vmi->vmi_base;
	VM_UNLOCK(vm);
	return (0);
}

struct vm_index *
vm_find_index(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *iter;
	struct vm_index *vmi;

	VM_LOCK(vm);
	BTREE_FIND(&vmi, iter, vm->vm_index, vmi_tree, (vaddr < iter->vmi_base),
		   ((vaddr == iter->vmi_base) ||
		    (vaddr > iter->vmi_base &&
		     vaddr < (iter->vmi_base + PAGE_TO_ADDR(iter->vmi_size)))));
	VM_UNLOCK(vm);
	return (vmi);
}

int
vm_free_address(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *vmi;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi != NULL) {
		ASSERT(TAILQ_EMPTY(&vmi->vmi_map),
		       "Cannot free address with active mappings.");
		vm_free_index(vm, vmi);
		VM_UNLOCK(vm);
		return (0);
	}
	VM_UNLOCK(vm);
	return (ERROR_NOT_FOUND);
}

int
vm_insert_range(struct vm *vm, vaddr_t begin, vaddr_t end)
{
	struct vm_index *iter;
	struct vm_index *vmi;

	vmi = pool_allocate(&vm_index_pool);
	if (vmi == NULL)
		return (ERROR_EXHAUSTED);
	VM_LOCK(vm);
	vmi->vmi_base = begin;
	vmi->vmi_size = ADDR_TO_PAGE(end - begin);
	BTREE_INIT(&vmi->vmi_tree);
	TAILQ_INIT(&vmi->vmi_map);
	vm_free_index(vm, vmi);
	/* XXX Push in to btree_insert.  */
	if (vm->vm_index == NULL) {
		vm->vm_index = vmi;
		VM_UNLOCK(vm);
		return (0);
	}
	BTREE_INSERT(vmi, iter, vm->vm_index, vmi_tree,
		     (vmi->vmi_base < iter->vmi_base));
	VM_UNLOCK(vm);
	return (0);
}

int
vm_page_map(struct vm *vm, struct vm_page *page, vaddr_t *vaddrp)
{
	int error, error2;

	error = vm_alloc_address(vm, vaddrp, 1);
	if (error != 0)
		return (error);
	error = page_map(vm, *vaddrp, page);
	if (error != 0) {
		error2 = vm_free_address(vm, *vaddrp);
		if (error2 != 0)
			panic("%s: vm_free_address failed: %m", __func__,
			      error2);
		return (error);
	}
	return (0);
}

int
vm_page_unmap(struct vm *vm, vaddr_t vaddr)
{
	int error;

	error = page_unmap(vm, vaddr);
	if (error != 0)
		return (error);
	error = vm_free_address(vm, vaddr);
	if (error != 0)
		return (error);
	return (0);
}

static void
vm_free_index(struct vm *vm, struct vm_index *vmi)
{
	TAILQ_INSERT_HEAD(&vm->vm_index_free, vmi, vmi_free_link);
}

static int
vm_use_index(struct vm *vm, struct vm_index *vmi, size_t pages)
{
	size_t count;
	int error;

	TAILQ_REMOVE(&vm->vm_index_free, vmi, vmi_free_link);
	if (vmi->vmi_size != pages) {
		count = vmi->vmi_size - pages;
		vmi->vmi_size = pages;
		error = vm_insert_range(vm, vmi->vmi_base + (pages * PAGE_SIZE),
					vmi->vmi_base + ((pages + count) *
							 PAGE_SIZE));
		if (error != 0) {
			vmi->vmi_size += count;
			vm_free_index(vm, vmi);
			return (error);
		}
	}
	return (0);
}
static void
db_vm_index_dump(struct vm_index *vmi)
{
	struct vm_map_page *vmmp;

	/* XXX DOT
	kcprintf("VMI%p [ label=\"%p\" ];\n", vmi, vmi->vmi_base);
	kcprintf("VMI%p -> VMI%p;\n", vmi, vmi->vmi_tree.left);
	kcprintf("VMI%p -> VMI%p;\n", vmi, vmi->vmi_tree.right);
	return;
	*/
	kcprintf("VM Index %p [ %p ... %p (%zu pages) ]\n", vmi,
		 (void *)vmi->vmi_base,
		 (void *)(vmi->vmi_base + vmi->vmi_size * PAGE_SIZE),
		 vmi->vmi_size);
	TAILQ_FOREACH(vmmp, &vmi->vmi_map, vmmp_link) {
		kcprintf("\tMapping %p -> %p (vm_page %p, vm_map_page %p)\n",
			 (void *)vmmp->vmmp_base,
			 (void *)page_address(vmmp->vmmp_page),
			 vmmp->vmmp_page, vmmp);
	}
}

static void
db_vm_index_dump_vm(struct vm *vm, const char *name)
{
	struct vm_index *vmi;

	kcprintf("Dumping %s VM Index...\n", name);
	BTREE_FOREACH(vmi, vm->vm_index, vmi_tree, db_vm_index_dump);
}

static void
db_vm_index_dump_kvm(void)
{
	db_vm_index_dump_vm(&kernel_vm, "Kernel");
}
DB_SHOW_VALUE_VOIDF(kvm, vm_index, db_vm_index_dump_kvm);

static void
db_vm_index_dump_task(void)
{
	if (current_thread() != NULL) {
		struct vm *vm;

		vm = current_thread()->td_parent->t_vm;
		db_vm_index_dump_vm(vm, "Thread");
	} else {
		kcprintf("No running thread.\n");
	}
}
DB_SHOW_VALUE_VOIDF(task, vm_index, db_vm_index_dump_task);

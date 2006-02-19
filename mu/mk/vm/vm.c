#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

struct vm_index {
	vaddr_t vmi_base;
	size_t vmi_size;
	struct vm_index *vmi_left;
	struct vm_index *vmi_right;
	struct vm_index *vmi_free_prev;
	struct vm_index *vmi_free_next;
};

static struct pool vm_index_pool;
struct vm kernel_vm;

static void vm_insert_index(struct vm_index *, struct vm_index *);

void
vm_init(void)
{
	int error;

	error = pool_create(&vm_index_pool, "VM Index",
			    sizeof (struct vm_index), POOL_DEFAULT);
	if (error != 0)
		panic("%s: failed to create pool: %u", __func__, error);
}

int
vm_alloc_address(struct vm *vm, vaddr_t *vaddrp, size_t pages)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_free_address(struct vm *vm, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_insert_range(struct vm *vm, vaddr_t begin, vaddr_t end)
{
	struct vm_index *vmi, *t;

	vmi = pool_allocate(&vm_index_pool);
	if (vmi == NULL)
		return (ERROR_EXHAUSTED);
	vmi->vmi_base = begin;
	vmi->vmi_size = PA_TO_PAGE(end - begin);
	vmi->vmi_left = NULL;
	vmi->vmi_right = NULL;
	vmi->vmi_free_prev = NULL;
	vmi->vmi_free_next = vm->vm_index_free;
	if (vm->vm_index_free != NULL)
		vm->vm_index_free->vmi_free_prev = vmi;
	vm->vm_index_free = vmi;
	if (vm->vm_index == NULL) {
		vm->vm_index = vmi;
		return (0);
	}
	vm_insert_index(vm->vm_index, vmi);
	return (0);
}

static void
vm_insert_index(struct vm_index *t, struct vm_index *vmi)
{
	panic("%s: too lazy to implement a tree right now!", __func__);
}

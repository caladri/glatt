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
static int vm_use_index(struct vm *, struct vm_index *, size_t);

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
	struct vm_index *vmi;
	struct vm_index *t;
	int error;

	vmi = NULL;

	/* Look for a best match.  */
	for (t = vm->vm_index_free; t != NULL; t = t->vmi_free_next) {
		if (t->vmi_size == pages) {
			error = vm_use_index(vm, t, pages);
			if (error != 0)
				return (error);
			*vaddrp = t->vmi_base;
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
		return (ERROR_NOT_IMPLEMENTED);
	}
	error = vm_use_index(vm, vmi, pages);
	if (error != 0)
		return (error);
	*vaddrp = vmi->vmi_base;
	return (0);
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
	struct vm_index **n;
	if (vmi->vmi_base < t->vmi_base)
		n = &t->vmi_left;
	else
		n = &t->vmi_right;
	if (*n != NULL)
		vm_insert_index(*n, vmi);
	else
		*n = vmi;
}

static int
vm_use_index(struct vm *vm, struct vm_index *vmi, size_t pages)
{
	size_t count;
	int error;

	if (vmi->vmi_free_prev != NULL) {
		vmi->vmi_free_prev->vmi_free_next = vmi->vmi_free_next;
	}
	if (vmi->vmi_free_next != NULL) {
		vmi->vmi_free_next->vmi_free_prev = vmi->vmi_free_prev;
	}
	if (vmi->vmi_size != pages) {
		count = vmi->vmi_size - pages;
		vmi->vmi_size = pages;
		error = vm_insert_range(vm, vmi->vmi_base + pages,
					vmi->vmi_base + pages + count);
		if (error != 0) {
			vmi->vmi_size += count;
			if (vmi->vmi_free_prev != NULL) {
				vmi->vmi_free_prev->vmi_free_next = vmi;
			}
			if (vmi->vmi_free_next != NULL) {
				vmi->vmi_free_next->vmi_free_prev = vmi;
			}
			return (error);
		}
	}
	return (0);
}

#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/task.h>
#include <core/thread.h>
#include <db/db.h>
#include <vm/index.h>
#include <vm/page.h>
#include <vm/vm.h>
#include <vm/index.h>
#include <vm/map.h>

static struct pool vm_map_page_pool;

static struct vm_map_page *vm_map_find(struct vm *,
						    struct vm_index *, vaddr_t);

int
vm_init_map(void)
{
	int error;

	error = pool_create(&vm_map_page_pool, "VM Map Page",
			    sizeof (struct vm_map_page), POOL_DEFAULT);
	if (error != 0)
		return (error);
	return (0);
}

int
vm_map_extract(struct vm *vm, vaddr_t vaddr, struct vm_page **pagep)
{
	struct vm_index *vmi;
	struct vm_map_page *vmmp;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	vmmp = vm_map_find(vm, vmi, vaddr);
	if (vmmp == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	*pagep = vmmp->vmmp_page;
	VM_UNLOCK(vm);
	return (0);
}

int
vm_map_free(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *vmi;
	struct vm_map_page *vmmp;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	vmmp = vm_map_find(vm, vmi, vaddr);
	if (vmmp == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	TAILQ_REMOVE(&vmi->vmi_map, vmmp, vmmp_link);
	pool_free(vmmp);
	VM_UNLOCK(vm);
	return (0);
}

int
vm_map_insert(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	struct vm_index *vmi;
	struct vm_map_page *vmmp;

	ASSERT(PAGE_ALIGNED(vaddr), "Page must be aligned.");

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	vmmp = pool_allocate(&vm_map_page_pool);
	if (vmmp == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_EXHAUSTED);
	}
	vmmp->vmmp_base = vaddr;
	vmmp->vmmp_page = page;
	TAILQ_INSERT_TAIL(&vmi->vmi_map, vmmp, vmmp_link);
	VM_UNLOCK(vm);
	return (0);
}

static struct vm_map_page *
vm_map_find(struct vm *vm, struct vm_index *vmi, vaddr_t vaddr)
{
	struct vm_map_page *vmmp;

	SPINLOCK_ASSERT_HELD(&vm->vm_lock);
	TAILQ_FOREACH(vmmp, &vmi->vmi_map, vmmp_link) {
		if (PAGE_FLOOR(vaddr) == vmmp->vmmp_base)
			return (vmmp);
	}
	return (NULL);
}

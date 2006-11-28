#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

struct vm_index_page {
	vaddr_t vmip_base;
	struct vm_page *vmip_page;
	TAILQ_ENTRY(struct vm_index_page) vmip_link;
};

struct vm_index {
	vaddr_t vmi_base;
	size_t vmi_size;
	struct vm_index *vmi_left;
	struct vm_index *vmi_right;
	TAILQ_ENTRY(struct vm_index) vmi_free_link;
	TAILQ_HEAD(, struct vm_index_page) vmi_page_queue;
};

static struct pool vm_index_page_pool;
static struct pool vm_index_pool;

static struct vm_index *vm_find_index(struct vm *, vaddr_t);
static struct vm_index_page *vm_find_index_page(struct vm *, struct vm_index *, vaddr_t);
static void vm_free_index(struct vm *, struct vm_index *);
static void vm_insert_index(struct vm_index *, struct vm_index *);
static int vm_use_index(struct vm *, struct vm_index *, size_t);

int
vm_init_index(void)
{
	int error;

	error = pool_create(&vm_index_page_pool, "VM Index Page",
			    sizeof (struct vm_index_page), POOL_DEFAULT);
	if (error != 0)
		return (error);
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

int
vm_free_address(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *vmi;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi != NULL) {
		ASSERT(TAILQ_EMPTY(&vmi->vmi_page_queue),
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
	struct vm_index *vmi, *t;

	vmi = pool_allocate(&vm_index_pool);
	if (vmi == NULL)
		return (ERROR_EXHAUSTED);
	VM_LOCK(vm);
	vmi->vmi_base = begin;
	vmi->vmi_size = ADDR_TO_PAGE(end - begin);
	vmi->vmi_left = NULL;
	vmi->vmi_right = NULL;
	TAILQ_INIT(&vmi->vmi_page_queue);
	vm_free_index(vm, vmi);
	if (vm->vm_index == NULL) {
		vm->vm_index = vmi;
		VM_UNLOCK(vm);
		return (0);
	}
	/*
	 * XXX It would be easier to just make this the new vm->vm_index all
	 * the time and put the existing tree to our left or right appropriately
	 */
	vm_insert_index(vm->vm_index, vmi);
	VM_UNLOCK(vm);
	return (0);
}

int
vm_map_extract(struct vm *vm, vaddr_t vaddr, struct vm_page **pagep)
{
	struct vm_index *vmi;
	struct vm_index_page *vmip;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	vmip = vm_find_index_page(vm, vmi, vaddr);
	if (vmip == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	*pagep = vmip->vmip_page;
	VM_UNLOCK(vm);
	return (0);
}

int
vm_map_free(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *vmi;
	struct vm_index_page *vmip;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	vmip = vm_find_index_page(vm, vmi, vaddr);
	if (vmip == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	TAILQ_REMOVE(&vmi->vmi_page_queue, vmip, vmip_link);
	pool_free(vmip);
	VM_UNLOCK(vm);
	return (0);
}

int
vm_map_insert(struct vm *vm, vaddr_t vaddr, struct vm_page *page)
{
	struct vm_index *vmi;
	struct vm_index_page *vmip;

	ASSERT(PAGE_ALIGNED(vaddr), "Page must be aligned.");

	VM_LOCK(vm);
	vmi = vm_find_index(vm, vaddr);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	vmip = pool_allocate(&vm_index_page_pool);
	if (vmip == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_EXHAUSTED);
	}
	vmip->vmip_base = vaddr;
	vmip->vmip_page = page;
	TAILQ_INSERT_TAIL(&vmi->vmi_page_queue, vmip, vmip_link);
	VM_UNLOCK(vm);
	return (0);
}

static struct vm_index *
vm_find_index(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *vmi;

	VM_LOCK(vm);
	for (vmi = vm->vm_index; vmi != NULL; ) {
		if (vmi->vmi_base == vaddr) {
			VM_UNLOCK(vm);
			return (vmi);
		} else if (vaddr > vmi->vmi_base &&
			   vaddr < (vmi->vmi_base +
				    PAGE_TO_ADDR(vmi->vmi_size))) {
			/*
			 * XXX
			 * Separate for eventual statistics.
			 */
			VM_UNLOCK(vm);
			return (vmi);
		}
		if (vaddr < vmi->vmi_base)
			vmi = vmi->vmi_left;
		else
			vmi = vmi->vmi_right;
	}
	VM_UNLOCK(vm);
	return (NULL);
}

/*
 * XXX
 * Assert that vm is locked.
 */
static struct vm_index_page *
vm_find_index_page(struct vm *vm, struct vm_index *vmi, vaddr_t vaddr)
{
	struct vm_index_page *vmip;

	TAILQ_FOREACH(vmip, &vmi->vmi_page_queue, vmip_link) {
		if (PAGE_FLOOR(vaddr) == vmip->vmip_base) {
			return (vmip);
		}
	}
	return (NULL);
}

static void
vm_free_index(struct vm *vm, struct vm_index *vmi)
{
	TAILQ_INSERT_HEAD(&vm->vm_index_free, vmi, vmi_free_link);
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

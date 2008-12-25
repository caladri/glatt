#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/task.h>
#include <core/thread.h>
#ifdef DB
#include <db/db_command.h>
#include <io/console/console.h>
#endif
#include <vm/index.h>
#include <vm/page.h>
#include <vm/vm.h>

struct vm_index {
	vaddr_t vmi_base;
	size_t vmi_size;
	unsigned vmi_flags;
	BTREE_NODE(struct vm_index) vmi_tree;
	TAILQ_ENTRY(struct vm_index) vmi_free_link;
};
#define	VM_INDEX_FLAG_DEFAULT	(0x00000000)
#define	VM_INDEX_FLAG_INUSE	(0x00000001)

#ifdef DB
DB_COMMAND_TREE(index, vm, vm_index);
#endif

static struct pool vm_index_pool;

static struct vm_index *vm_find_index(struct vm *, vaddr_t);
static void vm_free_index(struct vm *, struct vm_index *);
static int vm_insert_index(struct vm *, struct vm_index **, vaddr_t, size_t);
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
	int error;

	VM_LOCK(vm);
	TAILQ_FOREACH(vmi, &vm->vm_index_free, vmi_free_link) {
		if (vmi->vmi_size == pages) {
			error = vm_use_index(vm, vmi, pages);
			if (error != 0) {
				VM_UNLOCK(vm);
				return (error);
			}
			*vaddrp = vmi->vmi_base;
			VM_UNLOCK(vm);
			return (0);
		}
	}
	TAILQ_FOREACH(vmi, &vm->vm_index_free, vmi_free_link) {
		if (vmi->vmi_size > pages) {
			error = vm_alloc_range(vm, vmi->vmi_base,
					       vmi->vmi_base +
					       PAGE_TO_ADDR(pages));
			if (error != 0) {
				VM_UNLOCK(vm);
				return (error);
			}
			*vaddrp = vmi->vmi_base;
			VM_UNLOCK(vm);
			return (0);
		}
	}
	/* XXX Look for adjacent entries, collapse, etc..  */
	VM_UNLOCK(vm);
	return (ERROR_NOT_IMPLEMENTED);
}

int
vm_alloc_range(struct vm *vm, vaddr_t begin, vaddr_t end)
{
	struct vm_index *vmi, *nvmi;
	size_t leader, trailer;
	size_t range;
	size_t size;
	int error;

	range = end - begin;

	VM_LOCK(vm);
	vmi = vm_find_index(vm, begin);
	if (vmi == NULL) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FOUND);
	}
	if ((vmi->vmi_flags & VM_INDEX_FLAG_INUSE) != 0) {
		VM_UNLOCK(vm);
		return (ERROR_NOT_FREE);
	}
	size = PAGE_TO_ADDR(vmi->vmi_size);
	if (vmi->vmi_base + size < end) {
		/* XXX Compact entries.  */
		VM_UNLOCK(vm);
		return (ERROR_NOT_IMPLEMENTED);
	}
	leader = begin - vmi->vmi_base;
	trailer = size - (leader + range);
	ASSERT(range + leader + trailer == size,
	       "Leader, trailer and range calculations incorrect.");
	if (leader != 0) {
		vmi->vmi_size = ADDR_TO_PAGE(leader);
		error = vm_insert_index(vm, &nvmi, begin, ADDR_TO_PAGE(range));
		if (error != 0) {
			vmi->vmi_size = ADDR_TO_PAGE(size);
			VM_UNLOCK(vm);
			return (error);
		}
	} else {
		nvmi = vmi;
		nvmi->vmi_size = ADDR_TO_PAGE(range);
	}
	if (trailer != 0) {
		error = vm_insert_index(vm, NULL, end, ADDR_TO_PAGE(trailer));
		if (error != 0) {
			/* XXX
			 * We could just give the user more than they
			 * wanted but that could turn out to be very bad.  For
			 * now just fragment the map and error out.
			 */
			nvmi->vmi_size += ADDR_TO_PAGE(trailer);
			VM_UNLOCK(vm);
			return (error);
		}
	}
	error = vm_use_index(vm, nvmi, ADDR_TO_PAGE(range));
	if (error != 0)
		panic("%s: vm_use_index failed: %m", __func__, error);
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
#if 0
		ASSERT(TAILQ_EMPTY(&vmi->vmi_map),
		       "Cannot free address with active mappings.");
#endif

		/*
		 * XXX
		 * If there is a range immediately before or after this one
		 * and one or both are free, merge them.
		 */

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
	int error;

	VM_LOCK(vm);
	error = vm_insert_index(vm, NULL, begin, ADDR_TO_PAGE(end - begin));
	if (error != 0) {
		VM_UNLOCK(vm);
		return (error);
	}
	VM_UNLOCK(vm);
	return (0);
}

static struct vm_index *
vm_find_index(struct vm *vm, vaddr_t vaddr)
{
	struct vm_index *iter;
	struct vm_index *vmi;

	VM_LOCK(vm);
	BTREE_FIND(&vmi, iter, &vm->vm_index, vmi_tree,
		   (vaddr < iter->vmi_base),
		   ((vaddr == iter->vmi_base) ||
		    (vaddr > iter->vmi_base &&
		     vaddr < (iter->vmi_base + PAGE_TO_ADDR(iter->vmi_size)))));
	VM_UNLOCK(vm);
	return (vmi);
}

static void
vm_free_index(struct vm *vm, struct vm_index *vmi)
{
	ASSERT((vmi->vmi_flags & VM_INDEX_FLAG_INUSE) != 0,
	       "VM Index must be in use.");
	vmi->vmi_flags &= ~VM_INDEX_FLAG_INUSE;
	/*
	 * In theory this means the most-fragmented spaces will stay at the
	 * front of the queue and the giant chunk o' free vm space will stay
	 * at the back, the last to be considered for allocations.  Thrashes
	 * the caches, que sera sera.
	 */
	TAILQ_INSERT_HEAD(&vm->vm_index_free, vmi, vmi_free_link);
}

static int
vm_insert_index(struct vm *vm, struct vm_index **vmip, vaddr_t base,
		size_t pages)
{
	struct vm_index *iter;
	struct vm_index *vmi;

	SPINLOCK_ASSERT_HELD(&vm->vm_lock);

	ASSERT(vm_find_index(vm, base) == NULL,
	       "Cannot insert an index twice!");

	vmi = pool_allocate(&vm_index_pool);
	if (vmi == NULL)
		return (ERROR_EXHAUSTED);
	vmi->vmi_base = base;
	vmi->vmi_size = pages;
	vmi->vmi_flags = VM_INDEX_FLAG_DEFAULT;
	BTREE_NODE_INIT(&vmi->vmi_tree);

	TAILQ_INSERT_TAIL(&vm->vm_index_free, vmi, vmi_free_link);
	BTREE_INSERT(vmi, iter, &vm->vm_index, vmi_tree,
		     (vmi->vmi_base < iter->vmi_base));

	if (vmip != NULL)
		*vmip = vmi;
	return (0);
}

static int
vm_use_index(struct vm *vm, struct vm_index *vmi, size_t pages)
{
	ASSERT((vmi->vmi_flags & VM_INDEX_FLAG_INUSE) == 0,
	       "VM Index must not be in use.");
	vmi->vmi_flags |= VM_INDEX_FLAG_INUSE;
	TAILQ_REMOVE(&vm->vm_index_free, vmi, vmi_free_link);
	ASSERT(vmi->vmi_size == pages, "You must know how much you're using.");
	return (0);
}

#ifdef DB
static void
db_vm_index_dump_dot(struct vm_index *vmi)
{
	kcprintf("VMI%p [ label=\"%p ... %p\" ];\n", vmi, vmi->vmi_base,
		 vmi->vmi_base + PAGE_TO_ADDR(vmi->vmi_size));
	kcprintf("VMI%p -> VMI%p;\n", vmi, vmi->vmi_tree.left);
	kcprintf("VMI%p -> VMI%p;\n", vmi, vmi->vmi_tree.right);
	/* XXX Should be easy to generate box-drawings via pmap.  */
}

static void
db_vm_index_dump(struct vm_index *vmi)
{
	kcprintf("VM Index %p [ %p ... %p (%zu pages) ]\n", vmi,
		 (void *)vmi->vmi_base,
		 (void *)(vmi->vmi_base + vmi->vmi_size * PAGE_SIZE),
		 vmi->vmi_size);
	/* XXX Show mappinga via pmap.  */
}

static void
db_vm_index_dump_vm(struct vm *vm, void (*function)(struct vm_index *))
{
	struct vm_index *vmi;

	BTREE_FOREACH(vmi, &vm->vm_index, vmi_tree, (function(vmi)));
}

static void
db_vm_index_dump_kvm(void)
{
	db_vm_index_dump_vm(&kernel_vm, db_vm_index_dump);
}
DB_COMMAND(kvm, vm_index, db_vm_index_dump_kvm);

static void
db_vm_index_dump_kvm_dot(void)
{
	kcprintf("digraph KVM {\n");
	db_vm_index_dump_vm(&kernel_vm, db_vm_index_dump_dot);
	kcprintf("};\n");
}
DB_COMMAND(dotkvm, vm_index, db_vm_index_dump_kvm_dot);

static void
db_vm_index_dump_task(void)
{
	if (current_task() != NULL) {
		struct vm *vm;

		vm = current_task()->t_vm;
		db_vm_index_dump_vm(vm, db_vm_index_dump_dot);
	} else {
		kcprintf("No running thread.\n");
	}
}
DB_COMMAND(task, vm_index, db_vm_index_dump_task);
#endif

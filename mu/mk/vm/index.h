#ifndef	_VM_INDEX_H_
#define	_VM_INDEX_H_

#include <core/btree.h>
#include <core/queue.h>
#include <vm/page.h>

struct vm_index {
	vaddr_t vmi_base;
	size_t vmi_size;
	BTREE(struct vm_index) vmi_tree;
	TAILQ_ENTRY(struct vm_index) vmi_free_link;
	TAILQ_HEAD(, struct vm_map_page) vmi_map;
};

int vm_init_index(void);

int vm_alloc_address(struct vm *, vaddr_t *, size_t);
struct vm_index *vm_find_index(struct vm *, vaddr_t);
int vm_free_address(struct vm *, vaddr_t);
int vm_insert_range(struct vm *, vaddr_t, vaddr_t);

#endif /* !_VM_INDEX_H_ */

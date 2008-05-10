#ifndef	_VM_MAP_H_
#define	_VM_MAP_H_

#include <vm/page.h>

struct vm_map_page {
	vaddr_t vmmp_base;
	struct vm_page *vmmp_page;
	TAILQ_ENTRY(struct vm_map_page) vmmp_link;
};

int vm_init_map(void);

int vm_map_extract(struct vm *, vaddr_t, struct vm_page **);
int vm_map_free(struct vm *, vaddr_t);
int vm_map_insert(struct vm *, vaddr_t, struct vm_page *);

#endif /* !_VM_MAP_H_ */

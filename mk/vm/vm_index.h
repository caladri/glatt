#ifndef	_VM_VM_INDEX_H_
#define	_VM_VM_INDEX_H_

int vm_init_index(void);

int vm_alloc_address(struct vm *, vaddr_t *, size_t);
int vm_alloc_range(struct vm *, vaddr_t, vaddr_t);
int vm_free_address(struct vm *, vaddr_t);
int vm_insert_range(struct vm *, vaddr_t, vaddr_t);

#endif /* !_VM_VM_INDEX_H_ */

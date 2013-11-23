#ifndef	_VM_VM_INDEX_H_
#define	_VM_VM_INDEX_H_

int vm_init_index(void);

int vm_alloc_address(struct vm *, vaddr_t *, size_t, bool) __non_null(1, 2) __check_result;
int vm_alloc_range(struct vm *, vaddr_t, vaddr_t) __non_null(1) __check_result;
int vm_free_address(struct vm *, vaddr_t) __non_null(1) __check_result;
int vm_insert_range(struct vm *, vaddr_t, vaddr_t) __non_null(1) __check_result;

#endif /* !_VM_VM_INDEX_H_ */

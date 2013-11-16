#ifndef	_VM_VM_ALLOC_H_
#define	_VM_VM_ALLOC_H_

struct vm;

	/* Virtual memory allocator.  */
int vm_alloc(struct vm *, size_t, vaddr_t *) __non_null(1, 3) __check_result;
int vm_alloc_page(struct vm *, vaddr_t *) __non_null(1, 2) __check_result;
int vm_alloc_range_wire(struct vm *, vaddr_t, vaddr_t, vaddr_t *, size_t *)
	__non_null(1, 4, 5) __check_result;
int vm_free(struct vm *, size_t, vaddr_t) __non_null(1) __check_result;
int vm_free_page(struct vm *, vaddr_t) __non_null(1) __check_result;

	/* Virtual memory wiring without allocation.  */
int vm_wire(struct vm *, vaddr_t, size_t, vaddr_t *, size_t *)
	__non_null(1, 4, 5) __check_result;
int vm_unwire(struct vm *, vaddr_t, size_t, vaddr_t)
	__non_null(1) __check_result;

#endif /* !_VM_VM_ALLOC_H_ */

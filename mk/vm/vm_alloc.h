#ifndef	_VM_VM_ALLOC_H_
#define	_VM_VM_ALLOC_H_

struct vm;

	/* Virtual memory allocator.  */
int vm_alloc(struct vm *, size_t, vaddr_t *) __non_null(1, 3);
int vm_alloc_page(struct vm *, vaddr_t *) __non_null(1, 2);
int vm_alloc_range_wire(struct vm *, vaddr_t, vaddr_t, vaddr_t *)
	__non_null(1, 4);
int vm_free(struct vm *, size_t, vaddr_t) __non_null(1);
int vm_free_page(struct vm *, vaddr_t) __non_null(1);

#endif /* !_VM_VM_ALLOC_H_ */

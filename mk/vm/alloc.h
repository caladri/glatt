#ifndef	_VM_ALLOC_H_
#define	_VM_ALLOC_H_

struct vm;

	/* Virtual memory allocator.  */
int vm_alloc(struct vm *, size_t, vaddr_t *);
int vm_alloc_page(struct vm *, vaddr_t *);
int vm_free(struct vm *, size_t, vaddr_t);
int vm_free_page(struct vm *, vaddr_t);

#endif /* !_VM_ALLOC_H_ */

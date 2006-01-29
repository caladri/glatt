#ifndef	_VM_PAGE_H_
#define	_VM_PAGE_H_

#include <page/page.h>

struct vm;

#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)
#define	PA_TO_PAGE(pa)	((pa) >> PAGE_SHIFT)
#define	PAGE_TO_PA(pa)	((pa) << PAGE_SHIFT)
#define	PAGE_FLOOR(a)	((a) & ~PAGE_MASK)
#define	PAGE_OFFSET(a)	((a) & PAGE_MASK)

	/* It's grim up north.  */

int page_alloc(struct vm *, paddr_t *);
int page_alloc_virtual(struct vm *, vaddr_t *);
int page_insert(paddr_t);
int page_map(struct vm *, vaddr_t, paddr_t);
int page_release(struct vm *, paddr_t);
int page_unmap(struct vm *, vaddr_t);

#endif /* !_VM_PAGE_H_ */

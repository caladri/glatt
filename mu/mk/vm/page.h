#ifndef	_VM_PAGE_H_
#define	_VM_PAGE_H_

#include <page/page.h>

struct vm;

#define	PAGE_SIZE		(1 << PAGE_SHIFT)
#define	PAGE_MASK		(PAGE_SIZE - 1)
#define	ADDR_TO_PAGE(a)		((a) >> PAGE_SHIFT)
#define	PAGE_TO_ADDR(a)		((a) << PAGE_SHIFT)
#define	PAGE_FLOOR(a)		((a) & ~PAGE_MASK)
#define	PAGE_OFFSET(a)		((a) & PAGE_MASK)

#define	PAGE_FLAG_DEFAULT	(0x00000000)
#define	PAGE_FLAG_ZERO		(0x00000001)

	/* It's grim up north.  */

void page_init(void);

int page_alloc(struct vm *, uint32_t, paddr_t *);
int page_alloc_direct(struct vm *, uint32_t, vaddr_t *);
int page_extract(struct vm *, vaddr_t, paddr_t *);
int page_free_direct(struct vm *, vaddr_t);
int page_insert_pages(paddr_t, size_t);
int page_map(struct vm *, vaddr_t, paddr_t);
int page_map_direct(struct vm *, paddr_t, vaddr_t *);
int page_release(struct vm *, paddr_t);
int page_unmap(struct vm *, vaddr_t);
int page_unmap_direct(struct vm *, vaddr_t);
void page_zero(paddr_t);

#endif /* !_VM_PAGE_H_ */

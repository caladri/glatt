#ifndef	_VM_PAGE_H_
#define	_VM_PAGE_H_

#include <core/queue.h>
#include <cpu/page.h>

struct vm;
struct vm_page;

struct vm_page {
	TAILQ_ENTRY(struct vm_page) pg_link;
	unsigned pg_refcnt;
};
TAILQ_HEAD(vm_pageq, struct vm_page);

#define	PAGE_SIZE		(1 << PAGE_SHIFT)
#define	PAGE_MASK		(PAGE_SIZE - 1)
#define	ADDR_TO_PAGE(a)		((a) >> PAGE_SHIFT)
#define	PAGE_TO_ADDR(a)		((a) << PAGE_SHIFT)
#define	PAGE_FLOOR(a)		((a) & ~PAGE_MASK)
#define	PAGE_OFFSET(a)		((a) & PAGE_MASK)
#define	PAGE_ALIGNED(a)		(PAGE_OFFSET(a) == 0)

#define	PAGE_FLAG_DEFAULT	(0x00000000)
#define	PAGE_FLAG_ZERO		(0x00000001)

	/* It's grim up north.  */

void page_init(void);

paddr_t page_address(struct vm_page *);
int page_alloc(unsigned, struct vm_page **);
int page_alloc_direct(struct vm *, unsigned, vaddr_t *);
int page_extract(struct vm *, vaddr_t, struct vm_page **);
int page_free_direct(struct vm *, vaddr_t);
int page_insert_pages(paddr_t, size_t);
int page_map(struct vm *, vaddr_t, struct vm_page *);
int page_map_direct(struct vm *, struct vm_page *, vaddr_t *);
int page_release(struct vm_page *);
int page_unmap(struct vm *, vaddr_t, struct vm_page *);
int page_unmap_direct(struct vm *, struct vm_page *, vaddr_t);

#endif /* !_VM_PAGE_H_ */

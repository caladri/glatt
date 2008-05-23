#ifndef	_PAGE_PAGE_H_
#define	_PAGE_PAGE_H_

	/* Machine-independent PMAP API.  */

#include <page/types.h>

struct vm;
struct vm_page;

#define	PAGE_SHIFT	(13)

void pmap_bootstrap(void);
int pmap_extract(struct vm *, vaddr_t, paddr_t *);
int pmap_init(struct vm *, vaddr_t, vaddr_t);
int pmap_map(struct vm *, vaddr_t, struct vm_page *);
int pmap_map_direct(struct vm *, paddr_t, vaddr_t *);
int pmap_unmap(struct vm *, vaddr_t);
int pmap_unmap_direct(struct vm *, vaddr_t);
void pmap_zero(struct vm_page *);

#endif /* !_PAGE_PAGE_H_ */

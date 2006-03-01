#ifndef	_PAGE_PAGE_H_
#define	_PAGE_PAGE_H_

	/* Machine-independent PMAP API.  */

#include <page/types.h>

struct vm;

#define	PAGE_SHIFT	(13)

void pmap_bootstrap(void);
int pmap_extract(struct vm *, vaddr_t, paddr_t *);
int pmap_init(struct vm *, bool);
int pmap_map(struct vm *, vaddr_t, paddr_t);
int pmap_map_direct(struct vm *, paddr_t, vaddr_t *);
int pmap_unmap(struct vm *, vaddr_t);
int pmap_unmap_direct(struct vm *, vaddr_t);

#endif /* !_PAGE_PAGE_H_ */

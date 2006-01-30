#ifndef	_PAGE_PAGE_H_
#define	_PAGE_PAGE_H_

#include <page/types.h>

struct vm;

#define	PAGE_SHIFT	(13)

int pmap_map_direct(struct vm *, paddr_t, vaddr_t *);

#endif /* !_PAGE_PAGE_H_ */

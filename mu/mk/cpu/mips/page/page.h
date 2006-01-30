#ifndef	_PAGE_PAGE_H_
#define	_PAGE_PAGE_H_

#ifndef	ASSEMBLER
#include <page/types.h>

struct vm;
#endif

#define	PAGE_SHIFT	(13)

#ifndef	ASSEMBLER
int pmap_map(struct vm *, vaddr_t, paddr_t);
int pmap_map_direct(struct vm *, paddr_t, vaddr_t *);
int pmap_unmap(struct vm *, vaddr_t);
#endif

#endif /* !_PAGE_PAGE_H_ */

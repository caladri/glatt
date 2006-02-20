#ifndef	_PAGE_PAGE_H_
#define	_PAGE_PAGE_H_

#ifndef	ASSEMBLER
#include <page/types.h>

struct pmap; /* Internal to page_map.c.  */
struct vm;
#endif

#define	PAGE_SHIFT	(13)

#ifndef	ASSEMBLER
unsigned pmap_asid(struct vm *);
void pmap_bootstrap(void);
int pmap_extract(struct vm *, vaddr_t, paddr_t *);
int pmap_map(struct vm *, vaddr_t, paddr_t);
int pmap_map_direct(struct vm *, paddr_t, vaddr_t *);
int pmap_unmap(struct vm *, vaddr_t);
int pmap_unmap_direct(struct vm *, vaddr_t);
#endif

#endif /* !_PAGE_PAGE_H_ */

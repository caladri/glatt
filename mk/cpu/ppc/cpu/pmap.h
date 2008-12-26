#ifndef	_CPU_PMAP_H_
#define	_CPU_PMAP_H_

#include <cpu/segment.h>

struct vm;
struct vm_page;

/* XXX */
struct pmap {
	vaddr_t pm_base;
	vaddr_t pm_end;
};

	/* Machine-independent PMAP API.  */

static inline int
pmap_extract_direct(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static inline int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static inline int
pmap_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

void pmap_bootstrap(void);
int pmap_extract(struct vm *, vaddr_t, paddr_t *);
#if 0
int pmap_extract_direct(struct vm *, vaddr_t, paddr_t *);
#endif
int pmap_init(struct vm *, vaddr_t, vaddr_t);
int pmap_map(struct vm *, vaddr_t, struct vm_page *);
#if 0
int pmap_map_direct(struct vm *, paddr_t, vaddr_t *);
#endif
int pmap_unmap(struct vm *, vaddr_t);
#if 0
int pmap_unmap_direct(struct vm *, vaddr_t);
#endif
void pmap_zero(struct vm_page *);

#endif /* !_CPU_PMAP_H_ */

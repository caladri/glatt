#ifndef	_CPU_PMAP_H_
#define	_CPU_PMAP_H_

	/* Machine-independent PMAP API.  */

#include <cpu/memory.h>

struct vm;
struct vm_page;

#define	PMAP_ASID_RESERVED	(0)
#define	PMAP_ASID_FIRST		(1)
#define	PMAP_ASID_MAX		(255)

/*
 * A quicker extract routine when the page layer knows the address is in the
 * direct-mapped region.  If we ever support pages which cannot be direct-mapped
 * (i.e. due to being in high memory) then this will need to handle them or we
 * need to teach the PAGE layer what can and cannot be direct-mapped.
 *
 * Specialization is evil, it should be possible to #define pmap_extract_direct
 * to pmap_extract.
 */
static inline int
pmap_extract_direct(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
#ifdef _VM_VM_H_
	ASSERT(vm == &kernel_vm, "Only the kernel VM can direct-map.");
#endif
	ASSERT(vaddr >= XKPHYS_BASE && vaddr <= XKPHYS_END,
	       "Direct-mapped address must be in range.");
	*paddrp = XKPHYS_EXTRACT(vaddr);
	return (0);
}

static inline int
pmap_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
#ifdef _VM_VM_H_
	ASSERT(vm == &kernel_vm, "Only the kernel VM can direct-map.");
#endif
	/* XXX Ensure that we can direct-map this address.  */
	*vaddrp = (vaddr_t)XKPHYS_MAP(CCA_CNC, paddr);
	return (0);
}

static inline int
pmap_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
#ifdef _VM_VM_H_
	ASSERT(vm == &kernel_vm, "Only the kernel VM can direct-map.");
#endif
	ASSERT(vaddr >= XKPHYS_BASE && vaddr <= XKPHYS_END,
	       "Direct-mapped address must be in range.");
	return (0);
}

void pmap_activate(struct vm *);
void pmap_bootstrap(void);
int pmap_extract(struct vm *, vaddr_t, paddr_t *) __non_null(1, 3);
int pmap_init(struct vm *, vaddr_t, vaddr_t) __non_null(1);
int pmap_map(struct vm *, vaddr_t, struct vm_page *) __non_null(1, 3);
int pmap_unmap(struct vm *, vaddr_t) __non_null(1);
void pmap_zero(struct vm_page *) __non_null(1);

#endif /* !_CPU_PMAP_H_ */

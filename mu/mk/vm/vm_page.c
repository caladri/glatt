#include <core/types.h>
#include <core/error.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

/*
 * XXX have a few page_index in each page of memory so that we don't have to
 * iterate through a large PAGE_INDEX_ENTRIES entries.  We can quickly check
 * whether each page_index has *any* pages, but if it only has 1 page in the
 * last place we're going to check, then that's very inefficient.  Keeping a
 * hint of the last place we set a bit (or took a bit but there were others
 * still set) would be useful, too.
 */

struct page_index;

struct page_header {
	struct page_index *ph_next;
	paddr_t ph_base;
	size_t ph_pages;
};

struct page_entry {
	uint64_t pe_bitmask;
};

#define	PAGE_INDEX_SPLIT	(8)
#define	PAGE_ENTRY_PAGES	(sizeof (struct page_entry) * 8)
#define	PAGE_INDEX_ENTRIES	(((PAGE_SIZE / PAGE_INDEX_SPLIT) -	\
				  sizeof (struct page_header)) /	\
				 sizeof (struct page_entry))
#define	PAGE_INDEX_COUNT	(PAGE_INDEX_ENTRIES * PAGE_ENTRY_PAGES)
static struct page_index {
	struct page_header pi_header;
	struct page_entry pi_entries[PAGE_INDEX_ENTRIES];
} *page_index;

COMPILE_TIME_ASSERT(sizeof (struct page_index) * PAGE_INDEX_SPLIT == PAGE_SIZE);

void
page_init(void)
{
	kcprintf("PAGE: page size is %uK, %u pages per index entry.\n",
		 PAGE_SIZE / 1024, PAGE_INDEX_COUNT);
}

int
page_alloc(struct vm *vm, paddr_t *paddrp)
{
	struct page_entry *pe;
	struct page_index *pi;
	size_t entry, off;
	paddr_t paddr;

	for (pi = page_index; pi != NULL; pi = pi->pi_header.ph_next) {
		if (pi->pi_header.ph_pages == 0)
			continue;
		for (entry = 0; entry < PAGE_INDEX_ENTRIES; entry++) {
			pe = &pi->pi_entries[entry];
			if (pe->pe_bitmask == 0)
				continue;
			for (off = 0; off < PAGE_ENTRY_PAGES; off++) {
				if ((pe->pe_bitmask & (1 << off)) == 0)
					continue;
				pe->pe_bitmask ^= 1 << off;
				pi->pi_header.ph_pages--;
				paddr = pi->pi_header.ph_base;
				paddr += (entry * PAGE_ENTRY_PAGES) * PAGE_SIZE;
				paddr += off * PAGE_SIZE;
				*paddrp = paddr;
				return (0);
			}
		}
		panic("%s: page index has pages but no bits set.", __func__);
	}
	return (ERROR_EXHAUSTED);
}

int
page_alloc_direct(struct vm *vm, vaddr_t *vaddrp)
{
	paddr_t paddr;
	vaddr_t vaddr;
	int error, error2;

	error = page_alloc(vm, &paddr);
	if (error != 0)
		return (error);

	error = page_map_direct(vm, paddr, &vaddr);
	if (error != 0) {
		error2 = page_release(vm, paddr);
		if (error2 != 0)
			panic("%s: can't release paddr: %u", __func__, error);
		return (error);
	}
	*vaddrp = vaddr;
	return (0);
}

int
page_extract(struct vm *vm, vaddr_t vaddr, paddr_t *paddrp)
{
	return (pmap_extract(vm, vaddr, paddrp));
}

int
page_free_direct(struct vm *vm, vaddr_t vaddr)
{
	paddr_t paddr;
	int error;

	error = page_extract(vm, vaddr, &paddr);
	if (error != 0)
		return (error);

	error = page_unmap_direct(vm, vaddr);
	if (error != 0)
		return (error);

	error = page_release(vm, paddr);
	if (error != 0)
		return (error);

	return (0);
}

int
page_insert_pages(paddr_t base, size_t pages)
{
	struct page_index *pi;
	vaddr_t va;
	size_t cnt, pix;
	size_t inserted, indexcnt;
	int error;

	inserted = indexcnt = 0;

	/*
	 * XXX check if these pages belong in an existing pool.
	 */
	while (pages != 0) {
		error = page_map_direct(&kernel_vm, base, &va);
		if (error != 0)
			panic("%s: couldn't map page index directly: %d\n",
			      __func__, error);
		base += PAGE_SIZE;
		pages--;
		indexcnt += PAGE_INDEX_SPLIT;

		for (pix = 0; pix < PAGE_INDEX_SPLIT; pix++) {
			pi = &((struct page_index *)va)[pix];
			pi->pi_header.ph_next = page_index;
			page_index = pi;
			pi->pi_header.ph_base = base;
			pi->pi_header.ph_pages = MIN(pages, PAGE_INDEX_COUNT);

			base += (pi->pi_header.ph_pages) * PAGE_SIZE;
			pages -= pi->pi_header.ph_pages;
			inserted += pi->pi_header.ph_pages;

			for (cnt = 0; cnt < PAGE_INDEX_ENTRIES; cnt++) {
				struct page_entry *pe;
				
				pe = &pi->pi_entries[cnt];
				pe->pe_bitmask = 0;
			}
			for (cnt = 0; cnt < pi->pi_header.ph_pages; cnt++) {
				struct page_entry *pe;

				pe = &pi->pi_entries[cnt / PAGE_ENTRY_PAGES];
				pe->pe_bitmask |= 1 << (cnt % PAGE_ENTRY_PAGES);
			}
		}
	}
	kcprintf("PAGE: inserted %lu pages (%lu indexes)\n",
		 inserted, indexcnt);
	return (0);
}

int
page_map(struct vm *vm, vaddr_t vaddr, paddr_t paddr)
{
	return (pmap_map(vm, vaddr, paddr));
}

int
page_map_direct(struct vm *vm, paddr_t paddr, vaddr_t *vaddrp)
{
	if (vm != &kernel_vm)
		panic("%s: can't direct map for non-kernel address space.",
		      __func__);
	return (pmap_map_direct(vm, paddr, vaddrp));
}

int
page_release(struct vm *vm, paddr_t paddr)
{
	struct page_entry *pe;
	struct page_index *pi;
	size_t off;

	for (pi = page_index; pi != NULL; pi = pi->pi_header.ph_next) {
		if (paddr < pi->pi_header.ph_base)
			continue;
		/*
		 * If this is not one first PAGE_INDEX_COUNT pages after the
		 * base address, then we should keep scanning.
		 */
		off = PA_TO_PAGE(paddr - pi->pi_header.ph_base);
		if (off >= PAGE_INDEX_COUNT)
			continue;
		pe = &pi->pi_entries[off / PAGE_ENTRY_PAGES];
		pe->pe_bitmask |= 1 << (off % PAGE_ENTRY_PAGES);
		pi->pi_header.ph_pages++;
		return (0);
	}
	return (ERROR_NOT_FOUND);
}

int
page_unmap(struct vm *vm, vaddr_t vaddr)
{
	return (pmap_unmap(vm, vaddr));
}

int
page_unmap_direct(struct vm *vm, vaddr_t vaddr)
{
	return (pmap_unmap_direct(vm, vaddr));
}

#include <core/types.h>
#include <core/error.h>
#include <core/macro.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

struct page_index;

struct page_header {
	struct page_index *ph_next;
	paddr_t ph_base;
	size_t ph_pages;
};

struct page_entry {
	uint64_t pe_bitmask;
};

#define	PAGE_ENTRY_PAGES	(sizeof (struct page_entry) * 8)
#define	PAGE_INDEX_ENTRIES	((PAGE_SIZE -				\
				  sizeof (struct page_header)) /	\
				 sizeof (struct page_entry))
#define	PAGE_INDEX_COUNT	(PAGE_INDEX_ENTRIES * PAGE_ENTRY_PAGES)
static struct page_index {
	struct page_header pi_header;
	struct page_entry pi_entries[PAGE_INDEX_ENTRIES];
} *page_index;

COMPILE_TIME_ASSERT(sizeof (struct page_index) == PAGE_SIZE);

void
page_init(void)
{
	kcprintf("PAGE: page size is %uK, %u pages per index entry.\n",
		 PAGE_SIZE / 1024, PAGE_INDEX_COUNT);
}

int
page_alloc(struct vm *vm, paddr_t *paddrp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
page_alloc_virtual(struct vm *vm, vaddr_t *vaddrp)
{
	paddr_t paddr;
	vaddr_t vaddr;
	int error, error2;

	error = page_alloc(vm, &paddr);
	if (error != 0)
		return (error);

	error = vm_find_address(vm, &vaddr);
	if (error != 0) {
		error2 = page_release(vm, paddr);
		if (error2 != 0) {
			panic("%s: can't release paddr.", __func__);
		}
		return (error);
	}

	error = page_map(vm, paddr, vaddr);
	if (error != 0) {
		error2 = vm_free_address(vm, vaddr);
		if (error2 != 0) {
			panic("%s: can't release vaddr.", __func__);
		}
		return (error);
	}
	*vaddrp = vaddr;
	return (0);
}

int
page_insert(paddr_t paddr)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
page_insert_pages(paddr_t base, size_t pages)
{
	struct page_index *pi;
	vaddr_t va;
	size_t cnt;
	int error;

	/*
	 * XXX check if these pages belong in an existing pool.
	 */
	while (pages != 0) {
		error = page_map_direct(&kernel_vm, base, &va);
		if (error != 0)
			panic("%s: couldn't map page index directly: %d\n",
			      __func__, error);
		pi = (struct page_index *)va;

		base += PAGE_SIZE;
		pages--;

		pi->pi_header.ph_next = page_index;
		page_index = pi;
		pi->pi_header.ph_base = base;
		pi->pi_header.ph_pages = MIN(pages, PAGE_INDEX_COUNT);

		base += (pi->pi_header.ph_pages) * PAGE_SIZE;
		pages -= pi->pi_header.ph_pages;

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
		 * If this is not one first PAGE_ENTRY_PAGES pages after the
		 * base address, then we should keep scanning.
		 */
		off = PA_TO_PAGE(paddr - pi->pi_header.ph_base);
		if (off >= PAGE_ENTRY_PAGES)
			continue;
		pe = &pi->pi_entries[off / PAGE_ENTRY_PAGES];
		pe->pe_bitmask |= 1 << (off % PAGE_ENTRY_PAGES);
		return (0);
	}
	return (ERROR_NOT_FOUND);
}

int
page_unmap(struct vm *vm, vaddr_t vaddr)
{
	return (pmap_unmap(vm, vaddr));
}

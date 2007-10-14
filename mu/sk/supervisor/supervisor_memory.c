#include <sk/types.h>
#include <sk/page.h>
#include <cpu/supervisor/memory.h>
#include <supervisor/memory.h>

struct pageinfo_header {
	paddr_t ph_base;
	size_t ph_bytes;
	struct pageinfo_header *ph_link;
};

struct pageinfo_page {
	void *pp_dummy;
};
#define	PAGES_PER_PAGEINFO						\
	((PAGE_SIZE - sizeof (struct pageinfo_header)) / sizeof (struct pageinfo_page))

struct pageinfo {
	struct pageinfo_header pi_header;
	struct pageinfo_page pi_pages[PAGES_PER_PAGEINFO];
};
COMPILE_TIME_ASSERT(sizeof (struct pageinfo) == PAGE_SIZE);

void *
supervisor_memory_direct_map(paddr_t physaddr)
{
	void *m;

	m = (void *)supervisor_cpu_memory_direct_map(physaddr);
	if (m == NULL) {
		/* XXX panic */
		return (NULL);
	}
	return (m);
}

void
supervisor_memory_insert(paddr_t base, size_t bytes,
			 enum supervisor_memory_type type)
{
	struct pageinfo *pi;
	unsigned i;

	if ((base & PAGE_MASK) != 0) {
		/* XXX printf */
		return;
	}
	if (bytes < (PAGE_SIZE * 2)) {
		/* XXX printf */
		return;
	}

	pi = supervisor_memory_direct_map(base);
	pi->pi_header.ph_base = base + PAGE_SIZE;
	pi->pi_header.ph_bytes = bytes - PAGE_SIZE;
	if (pi->pi_header.ph_bytes > PAGES_PER_PAGEINFO * PAGE_SIZE)
		pi->pi_header.ph_bytes = PAGES_PER_PAGEINFO * PAGE_SIZE;
	/*
	 * XXX
	 * pi->pi_header.ph_link = ???;
	 * Depends whether it's global or local memory!
	 */

	for (i = 0; i < PAGES_PER_PAGEINFO; i++) {
		struct pageinfo_page *pp;

		pp = &pi->pi_pages[i];
		if (i < pi->pi_header.ph_bytes / PAGE_SIZE) {
			pp->pp_dummy = pp;
		} else {
			/*
			 * This page is not present or not present in this
			 * array.
			 */
			pp->pp_dummy = NULL;
		}
	}
	supervisor_memory_insert(base + (PAGE_SIZE + pi->pi_header.ph_bytes),
				 bytes - (PAGE_SIZE + pi->pi_header.ph_bytes),
				 type);
}

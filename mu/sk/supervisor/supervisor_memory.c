#include <sk/types.h>
#include <sk/page.h>
#include <sk/queue.h>
#include <cpu/supervisor/memory.h>
#include <supervisor/memory.h>

struct pageinfo;

struct pageinfo_header {
	paddr_t ph_base;
	size_t ph_bytes;
	STAILQ_ENTRY(struct pageinfo) ph_link;
};

struct pageinfo_page {
	enum pageinfo_state {
		PAGEINFO_NOT_PRESENT,
		PAGEINFO_FREE,
		PAGEINFO_IN_USE,
	} pp_state;
};
#define	PAGES_PER_PAGEINFO						\
	((PAGE_SIZE - sizeof (struct pageinfo_header)) / sizeof (struct pageinfo_page))

struct pageinfo {
	struct pageinfo_header pi_header;
	struct pageinfo_page pi_pages[PAGES_PER_PAGEINFO];
};
COMPILE_TIME_ASSERT(sizeof (struct pageinfo) == PAGE_SIZE);
static STAILQ_HEAD(, struct pageinfo) supervisor_memory_global = 
	STAILQ_HEAD_INITIALIZER(supervisor_memory_global);

void *
supervisor_memory_alloc(size_t bytes)
{
	struct pageinfo_page *pp;
	struct pageinfo *pi;
	paddr_t physaddr;
	unsigned i;

	if (bytes > PAGE_SIZE)
		return (NULL);
	STAILQ_FOREACH(pi, &supervisor_memory_global, pi_header.ph_link) {
		for (i = 0; i < PAGES_PER_PAGEINFO; i++) {
			pp = &pi->pi_pages[i];
			/*
			 * If we hit a page that isn't present, then we are
			 * at the end of this pageinfo page - we don't
			 * support sparse memory maps.
			 */
			if (pp->pp_state == PAGEINFO_NOT_PRESENT)
				break;
			/*
			 * Look for a free page.
			 */
			if (pp->pp_state != PAGEINFO_FREE)
				continue;
			pp->pp_state = PAGEINFO_IN_USE;
			physaddr = pi->pi_header.ph_base;
			physaddr += i * PAGE_SIZE;
			return (supervisor_memory_direct_map(physaddr));
		}
	}
	return (NULL);
}

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
	switch (type) {
	case MEMORY_GLOBAL:
		STAILQ_INSERT_TAIL(&supervisor_memory_global,
				   pi, pi_header.ph_link);
		break;
	case MEMORY_LOCAL:
		/* XXX Find a proper local queue.  */
		return;
	}

	for (i = 0; i < PAGES_PER_PAGEINFO; i++) {
		struct pageinfo_page *pp;

		pp = &pi->pi_pages[i];
		if (i < pi->pi_header.ph_bytes / PAGE_SIZE) {
			pp->pp_state = PAGEINFO_FREE;
		} else {
			pp->pp_state = PAGEINFO_NOT_PRESENT;
		}
	}
	supervisor_memory_insert(base + (PAGE_SIZE + pi->pi_header.ph_bytes),
				 bytes - (PAGE_SIZE + pi->pi_header.ph_bytes),
				 type);
}

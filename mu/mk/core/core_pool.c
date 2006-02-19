#include <core/types.h>
#include <core/alloc.h>
#include <core/error.h>
#include <db/db.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	POOL_ITEM_DEFAULT	(0x0000)
#define	POOL_ITEM_FREE		(0x0001)

struct pool_item {
	uint16_t pi_flags;
	uint16_t pi_unused16;
	uint32_t pi_unused32;
};

struct pool_page {
	struct pool *pp_pool;
	struct pool_page *pp_next;
	size_t pp_items;
};

static struct pool_page *pool_page(struct pool_item *);
static struct pool_item *pool_get(struct pool *);

void *
pool_allocate(struct pool *pool)
{
	struct pool_item *item;
	struct pool_page *page;
	paddr_t page_addr;
	vaddr_t page_mapped;
	int error;

	item = pool_get(pool);
	if (item++ != NULL) {
		return ((void *)item);
	}
	error = page_alloc(&kernel_vm, &page_addr);
	if (error != 0) {
		/* XXX check whether we could block, try to GC some shit.  */
		return (NULL);
	}
	if ((pool->pool_flags & POOL_VIRTUAL) == 0) {
		error = vm_alloc_address(&kernel_vm, &page_mapped, PAGE_SIZE);
		if (error == 0) {
			error = page_map(&kernel_vm, page_addr, page_mapped);
			if (error != 0) {
				int error2;
				error2 = vm_free_address(&kernel_vm, page_addr);
				if (error2 != 0)
					panic("%s: vm_free_address failed: %u",
					      __func__, error);
			}
		}
	} else {
		error = page_map_direct(&kernel_vm, page_addr, &page_mapped);
	}
	if (error != 0)
		panic("%s: can't map page for allocation: %u", __func__, error);

	page = (struct pool_page *)page_mapped;
	page->pp_pool = pool;
	page->pp_next = pool->pool_pages;
	page->pp_items = 0;
	/* XXX initialize the pool_items.  */

	item = pool_get(pool);
	if (item++ == NULL)
		panic("%s: can't get memory but we just allocated!", __func__);
	return ((void *)item);
}

void
pool_free(struct pool *pool, void *m)
{
	struct pool_item *item;
	struct pool_page *page;

	item = m;
	item--;
	item->pi_flags |= POOL_ITEM_FREE;
	page = pool_page(item);
	if (page->pp_items-- == 1) {
		/* XXX free up page? */
		panic("%s: releasing last item.", __func__);
	}
}

int
pool_create(struct pool *pool, const char *name, size_t size, unsigned flags)
{
	pool->pool_name = name;
	pool->pool_size = 0;
	pool->pool_pages = NULL;
	pool->pool_flags = flags;
	return (0);
}

int
pool_destroy(struct pool *pool)
{
	panic("%s: can't destroy pools yet.", __func__);
	return (ERROR_NOT_IMPLEMENTED);
}

static struct pool_page *
pool_page(struct pool_item *item)
{
	uintptr_t page;

	page = (uintptr_t)item;
	page = PAGE_FLOOR(page);
	return ((struct pool_page *)page);
}

static struct pool_item *
pool_get(struct pool *pool)
{
	return (NULL);
}

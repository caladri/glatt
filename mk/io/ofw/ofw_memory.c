#include <core/types.h>
#include <core/error.h>
#include <io/console/console.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_functions.h>
#include <io/ofw/ofw_memory.h>
#include <vm/page.h>

#define	OFW_REGION_MAX	(12)

int
ofw_memory_init(paddr_t skipbegin, paddr_t skipend)
{
	ofw_word_t available[OFW_REGION_MAX * 2];
	ofw_word_t regions[OFW_REGION_MAX * 2];
	ofw_package_t memory;
	size_t alen, rlen;
	unsigned i;
	int error;

	error = ofw_finddevice("/memory", &memory);
	if (error != 0)
		return (error);

	error = ofw_getprop(memory, "reg", regions, sizeof regions, &rlen);
	if (error != 0)
		return (error);

	if (rlen > sizeof regions)
		panic("%s: too many regions.", __func__);

	error = ofw_getprop(memory, "available", available, sizeof available,
			    &alen);
	if (error != 0)
		return (error);

	if (alen > sizeof available)
		panic("%s: too many available regions.", __func__);

	for (i = 0; i < alen / sizeof available[0]; i += 2) {
		paddr_t begin = available[i + 0];
		paddr_t end = begin + available[i + 1];

		if (begin >= skipbegin && begin < skipend) {
			begin = skipend;
		}
		if (end >= skipbegin && end < skipend) {
			end = skipbegin;
		}
		if (begin >= end)
			continue;
		error = page_insert_pages(begin, ADDR_TO_PAGE(end - begin));
		if (error != 0)
			panic("%s: page_insert_pages failed: %m", __func__,
			      error);
	}
	return (0);
}

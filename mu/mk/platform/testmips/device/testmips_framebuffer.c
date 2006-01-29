#include <core/copyright.h>
#include <core/types.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <io/device/console/console.h>
#include <io/device/console/framebuffer.h>
#include <vm/page.h>

void
memcpy(void *dst, const void *src, size_t len)
{
	uint8_t *d = dst;
	const uint8_t *s = src;

	while (len--)
		*d++ = *s++;
}

static void
testmips_framebuffer_load(struct framebuffer *fb, struct bgr *bitmap)
{
	memcpy(fb->fb_softc, bitmap,
	       sizeof bitmap[0] * fb->fb_width * fb->fb_height);
}

static struct framebuffer testmips_framebuffer = {
	.fb_softc = XKPHYS_MAP(XKPHYS_UC, 0x12000000),
	.fb_load = testmips_framebuffer_load,
};

void
platform_start(void)
{
	/* XXX test::mp function memory() */
	uint64_t membytes = *(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, 0x11000000 | 0x0090);
	paddr_t offset;
	int error;

	framebuffer_init(&testmips_framebuffer, 640, 480);

	kcputs("\n");
	kcputs(MK_NAME "\n");
	kcputs(COPYRIGHT "\n");
	kcputs("\n");

	cpu_identify();

	offset = 0;
	while (membytes >= PAGE_SIZE) {
		error = page_insert(offset);
		if (error != 0)
			kcprintf("page_insert %#lx: %d\n", offset, error);

		offset += PAGE_SIZE;
		membytes -= PAGE_SIZE;
	}
	kcprintf("%u page(s).\n", PAGE_TO_PA(offset));
}

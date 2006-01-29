#include <core/types.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <io/device/console/console.h>
#include <io/device/console/framebuffer.h>

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
	framebuffer_init(&testmips_framebuffer, 640, 480);
	for (;;) {
		kcputs("Hello, world!\n");
	}
}

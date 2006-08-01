#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <io/device/console/framebuffer.h>

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

static void
testmips_framebuffer_startup(void *arg)
{
	struct framebuffer *fb;

	if (1)
		return;		/* Testing with framebuffer is pain.  */
	fb = arg;
	framebuffer_init(fb, 640, 480);
}
STARTUP_ITEM(testmips_framebuffer, STARTUP_DRIVERS, STARTUP_FIRST,
	     testmips_framebuffer_startup, &testmips_framebuffer);

#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <io/device/device.h>
#include <io/device/driver.h>
#include <io/device/console/framebuffer.h>

static void
tmfb_load(struct framebuffer *fb, struct bgr *bitmap)
{
	memcpy(fb->fb_softc, bitmap,
	       sizeof bitmap[0] * fb->fb_width * fb->fb_height);
}

static struct framebuffer tmfb_softc = {
	.fb_softc = XKPHYS_MAP(XKPHYS_UC, 0x12000000),
	.fb_load = tmfb_load,
};

static int
tmfb_probe(struct device *device)
{
	if (device->d_unit != 0)
		return (ERROR_NOT_FOUND);
#ifdef	FRAMEBUFFER
	return (0);
#else
	return (ERROR_NOT_IMPLEMENTED);
#endif
}

static int
tmfb_attach(struct device *device)
{
	struct framebuffer *fb = &tmfb_softc;

	device->d_softc = fb;
	/*
	 * XXX probe for resolution.
	 */
	framebuffer_init(fb, 640, 480);

	return (0);
}

DRIVER(tmfb, "testmips framebuffer", NULL, DRIVER_FLAG_DEFAULT, tmfb_probe, tmfb_attach);
DRIVER_ATTACHMENT(tmfb, "mp");

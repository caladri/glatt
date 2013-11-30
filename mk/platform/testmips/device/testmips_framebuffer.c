#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <io/bus/bus.h>
#include <io/framebuffer/framebuffer.h>

static void
tmfb_load(struct framebuffer *fb, const uint8_t *buffer)
{
	memcpy(fb->fb_softc, buffer,
	       3 * fb->fb_width * fb->fb_height);
}

static struct framebuffer tmfb_softc = {
	.fb_softc = XKPHYS_MAP(CCA_UC, 0x12000000),
	.fb_load = tmfb_load,
};

static int
tmfb_setup(struct bus_instance *bi)
{
	struct framebuffer *fb;

#ifndef	FRAMEBUFFER
	if (true)
		return (ERROR_NOT_IMPLEMENTED);
#endif

	bus_set_description(bi, "testmips simulated framebuffer");

	fb = bus_softc_allocate(bi, sizeof *fb);
	memcpy(fb, &tmfb_softc, sizeof *fb);
	/*
	 * XXX probe for resolution.
	 */
	framebuffer_init(fb, 640, 480);

	return (0);
}

BUS_INTERFACE(tmfbif) {
	.bus_setup = tmfb_setup,
};
BUS_ATTACHMENT(tmfb, "mpbus", tmfbif);

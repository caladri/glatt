#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/memory.h>
#include <io/bus/bus.h>
#include <io/framebuffer/framebuffer.h>

#define	TEST_FBCTRL_DEV_BASE	(0x12f00000)

#define	TEST_FBCTRL_DEV_PORT	(0x00)
#define	TEST_FBCTRL_DEV_DATA	(0x10)

#define	TEST_FBCTRL_DEV_FUNCTION(f)					\
	(volatile uint64_t *)XKPHYS_MAP(CCA_UC, TEST_FBCTRL_DEV_BASE + (f))
#define	TEST_FBCTRL_DEV_READ(f)						\
	(volatile uint64_t)*TEST_FBCTRL_DEV_FUNCTION(f)
#define	TEST_FBCTRL_DEV_WRITE(f, v)					\
	*TEST_FBCTRL_DEV_FUNCTION(f) = (v)

#define	TEST_FBCTRL_DEV_PORT_COMMAND	(0x00)
#define	TEST_FBCTRL_DEV_PORT_X1		(0x01)
#define	TEST_FBCTRL_DEV_PORT_Y1		(0x02)

#define	TEST_FBCTRL_DEV_COMMAND_SETRES	(0x01)
#define	TEST_FBCTRL_DEV_COMMAND_GETRES	(0x02)

#define	TMFB_RESOLUTION_MINWIDTH	(800)
#define	TMFB_RESOLUTION_MINHEIGHT	(600)

static void
tmfb_load(struct framebuffer *fb, const uint8_t *buffer, unsigned start, unsigned end)
{
	if (start == end)
		return;
	memcpy((uint8_t *)fb->fb_softc + start, buffer + start, end - start);
}

static struct framebuffer tmfb_softc = {
	.fb_softc = XKPHYS_MAP(CCA_UC, 0x12000000),
	.fb_load = tmfb_load,
};

static int
tmfb_setup(struct bus_instance *bi)
{
	unsigned xres, yres;
	struct framebuffer *fb;

	bus_set_description(bi, "testmips simulated framebuffer");

	fb = bus_softc_allocate(bi, sizeof *fb);
	memcpy(fb, &tmfb_softc, sizeof *fb);

	/* For some reason we're gettign 640x8192 as the resoltuion.  */
#if 0
	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_COMMAND);
	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, TEST_FBCTRL_DEV_COMMAND_GETRES);

	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_X1);
	xres = TEST_FBCTRL_DEV_READ(TEST_FBCTRL_DEV_DATA);

	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_Y1);
	yres = TEST_FBCTRL_DEV_READ(TEST_FBCTRL_DEV_DATA);

	if (xres < TMFB_RESOLUTION_MINWIDTH || yres < TMFB_RESOLUTION_MINHEIGHT) {
		if (xres < TMFB_RESOLUTION_MINWIDTH)
			xres = TMFB_RESOLUTION_MINWIDTH;
		if (yres < TMFB_RESOLUTION_MINHEIGHT)
			yres = TMFB_RESOLUTION_MINHEIGHT;

		TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_X1);
		TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, xres);

		TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_Y1);
		TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, yres);

		TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_COMMAND);
		TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, TEST_FBCTRL_DEV_COMMAND_SETRES);
	}
#else
	xres = TMFB_RESOLUTION_MINWIDTH;
	yres = TMFB_RESOLUTION_MINHEIGHT;

	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_X1);
	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, xres);

	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_Y1);
	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, yres);

	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_PORT, TEST_FBCTRL_DEV_PORT_COMMAND);
	TEST_FBCTRL_DEV_WRITE(TEST_FBCTRL_DEV_DATA, TEST_FBCTRL_DEV_COMMAND_SETRES);
#endif

	framebuffer_init(fb, xres, yres);

	return (0);
}

BUS_INTERFACE(tmfbif) {
	.bus_setup = tmfb_setup,
};
BUS_ATTACHMENT(tmfb, "mpbus", tmfbif);

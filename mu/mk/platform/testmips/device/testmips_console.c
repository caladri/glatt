#include <core/copyright.h>
#include <core/types.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/memory.h>
#include <db/db.h>
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

static void
testmips_console_putc(void *sc, char ch)
{
	volatile char *putcp = sc;
	*putcp = ch;
}

static void
testmips_console_flush(void *sc)
{
	(void)sc;
}

static struct console testmips_console = {
	.c_name = "testmips",
	.c_softc = XKPHYS_MAP(XKPHYS_UC, 0x10000000),
	.c_putc = testmips_console_putc,
	.c_flush = testmips_console_flush,
};

void
platform_start(void)
{
	static const int use_framebuffer = 0;
	/* XXX test::mp function memory() */
	uint64_t membytes = *(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, 0x11000000 | 0x0090);
	int error;

	error = mp_block_but_one(mp_whoami());
	if (error != 0) {
		/* XXX panic.  */
	}

	if (use_framebuffer)
		framebuffer_init(&testmips_framebuffer, 640, 480);
	else
		console_init(&testmips_console);

	kcputs("\n");
	kcputs(MK_NAME "\n");
	kcputs(COPYRIGHT "\n");
	kcputs("\n");

	/*
	 * Startup our physical page pool.
	 */
	page_init();

	/* 
	 * Add all global memory.  Processor-local memory will be added by
	 * the processor that owns it.
	 */
	error = page_insert_pages(0, PA_TO_PAGE(membytes));
	if (error != 0)
		panic("page_insert_pages 0..%lu failed: %d\n",
		      PA_TO_PAGE(membytes), error);

	platform_mp_start_all();	/* XXX doesn't return.  */
}

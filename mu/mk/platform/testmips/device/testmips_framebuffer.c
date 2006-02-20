#include <core/copyright.h>
#include <core/types.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <io/device/console/framebuffer.h>
#include <vm/page.h>
#include <vm/vm.h>

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
	size_t membytes;
	int error;

	/*
	 * XXX if other CPUs could be running, we should stop them here.  Sort
	 * out the mess later.
	 */

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
	 * the processor that owns it.  We skip the first 5MB of physical
	 * RAM because that's where the kernel will be loaded.  If we start
	 * to need more than 4MB, we're screwed.  Note that we have to load
	 * the kernel at 1MB to leave room for exception vectors and such
	 * at the start of physically-addressable memory.
	 */
#define	KERNEL_MAX_SIZE		(4 * 1024 * 1024)
#define	KERNEL_OFFSET		(1 * 1024 * 1024)
#define	KERNEL_PHYSICAL_HOLE	(KERNEL_MAX_SIZE + KERNEL_OFFSET)
	membytes = platform_mp_memory();
	if (membytes <= KERNEL_PHYSICAL_HOLE)
		panic("%s: not enough attached memory.");
	membytes -= KERNEL_PHYSICAL_HOLE;
	error = page_insert_pages(KERNEL_PHYSICAL_HOLE, PA_TO_PAGE(membytes));
	if (error != 0)
		panic("page_insert_pages %lu..%lu failed: %d",
		      PA_TO_PAGE(KERNEL_PHYSICAL_HOLE),
		      PA_TO_PAGE(membytes), error);

	/*
	 * Turn on exception handlers.  XXX we assume that only the boot CPU
	 * needs this done.  If that's not the case, then move this to
	 * the relevant place in platform_mp.c.  In any event, we need *this*
	 * CPU to have exception handlers right now, before we start up
	 * pmap.
	 */
	cpu_exception_init();

	/*
	 * Turn on the virtual memory subsystem.
	 */
	vm_init();

	vm_setup(&kernel_vm);

	pmap_bootstrap();
}

#include <core/types.h>
#include <core/copyright.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/memory.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <vm/page.h>
#include <vm/vm.h>

void
platform_halt(void)
{
	*(volatile int *)XKPHYS_MAP(XKPHYS_UC, 0x10000000 | 0x10) = 1;
	for (;;) {
		/* NOTREACHED */
	}
}

void
platform_start(void)
{
	extern void testmips_console_init(void); /* XXX */
	extern char __bss_start[], _end[];

	size_t membytes;
	int error;

	/*
	 * Initialize the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	/*
	 * XXX if other CPUs could be running, we should stop them here.  Sort
	 * out the mess later.
	 */

	testmips_console_init();

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
#define	KERNEL_MAX_SIZE		(4ul * 1024 * 1024)
#define	KERNEL_OFFSET		(1ul * 1024 * 1024)
#define	KERNEL_PHYSICAL_HOLE	(KERNEL_MAX_SIZE + KERNEL_OFFSET)
	membytes = platform_mp_memory();
	if (membytes <= KERNEL_PHYSICAL_HOLE)
		panic("%s: not enough attached memory.", __func__);
	membytes -= KERNEL_PHYSICAL_HOLE;
	error = page_insert_pages(KERNEL_PHYSICAL_HOLE, ADDR_TO_PAGE(membytes));
	if (error != 0)
		panic("page_insert_pages %lu..%lu failed: %m",
		      ADDR_TO_PAGE(KERNEL_PHYSICAL_HOLE),
		      ADDR_TO_PAGE(membytes), error);

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

	pmap_bootstrap();
}

#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/cpu.h>
#include <cpu/exception.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/startup.h>
#include <vm/vm_page.h>

extern char __bss_start[], _end[];

void
platform_halt(void)
{
	*(volatile int *)XKPHYS_MAP(CCA_UC, 0x10000000 | 0x10) = 1;
	NOTREACHED();
}

void
platform_start(void)
{
	size_t membytes;
	int error;

	/*
	 * Initialize the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	/*
	 * Set up the console.
	 */
	platform_console_init();

	/*
	 * Announce ourselves to the world.
	 */
	startup_version();

	/*
	 * Retrieve the amount of RAM available and steal a page for the PCPU
	 * data.  Skip the exception vectors and where the kernel is loaded.
	 */
#define	KERNEL_MAX_SIZE		(4ul * 1024 * 1024)
#define	KERNEL_OFFSET		(1ul * 1024 * 1024)
#define	KERNEL_PHYSICAL_HOLE	(KERNEL_MAX_SIZE + KERNEL_OFFSET)
#define	KERNEL_PHYSICAL_BASE	(KERNEL_PHYSICAL_HOLE + PAGE_SIZE)
	membytes = platform_mp_memory();
	if (membytes <= KERNEL_PHYSICAL_BASE)
		panic("%s: not enough attached memory.", __func__);
	if (((uintptr_t)_end & 0xffffffff00000000ul) != 0xffffffff00000000ul) {
		if (XKPHYS_EXTRACT(_end) >= KERNEL_PHYSICAL_BASE)
			panic("%s: kernel end is beyond physical hole (%p >= %p).",
			      __func__, (void *)XKPHYS_EXTRACT(_end),
			      (void *)KERNEL_PHYSICAL_BASE);
	} else {
		if (KSEG_EXTRACT(_end) >= KERNEL_PHYSICAL_BASE)
			panic("%s: kernel end is beyond physical hole (%p >= %p).",
			      __func__, (void *)KSEG_EXTRACT(_end),
			      (void *)KERNEL_PHYSICAL_BASE);
	}
	membytes -= KERNEL_PHYSICAL_BASE;

	/*
	 * Set up PCPU data, etc.  We'd like to do this earlier but can't yet.
	 */
	cpu_startup(KERNEL_PHYSICAL_HOLE);

	/*
	 * Turn on exception handlers.
	 */
	cpu_exception_init();

	/*
	 * Startup our physical page pool.
	 */
	page_init();

	/*
	 * XXX
	 * Need to skip any memory-mapped devices in this range.
	 */
	error = page_insert_pages(KERNEL_PHYSICAL_BASE, ADDR_TO_PAGE(membytes));
	if (error != 0)
		panic("page_insert_pages %lx..%lx failed: %m",
		      ADDR_TO_PAGE(KERNEL_PHYSICAL_BASE),
		      ADDR_TO_PAGE(membytes), error);

	/*
	 * Start everything that needs done before threading.
	 */
	startup_init();

	/*
	 * Now go on to MI startup.
	 */
	startup_main();
}

void
platform_startup_thread(void)
{
	platform_mp_startup();
}

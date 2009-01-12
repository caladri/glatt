#include <core/types.h>
#include <core/error.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/mp.h>
#include <cpu/interrupt.h>
#include <cpu/startup.h>
#ifdef DB
#include <db/db.h>
#endif
#include <device/ofw.h>
#include <io/console/console.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_functions.h>
#include <io/ofw/ofw_memory.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

extern char __bss_start[], _end[];

void
platform_halt(void)
{
	ofw_exit();
}

void
platform_start(register_t boot_args, register_t magic, register_t ofw_entry,
	       register_t argv, register_t argc)
{
	int error;

	/*
	 * Initialize the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	/*
	 * Set up OFW.  First set up the platform-specific bits and then tell
	 * the generic OFW code how to make an OFW call on this platform.  The
	 * generic OFW code will set up a console, too.
	 */
	platform_ofw_init(ofw_entry);

	ofw_init(macppc_ofw_call);

	/*
	 * Announce ourselves to the world.
	 */
	startup_version();

	/*
	 * Set up PCPU data, etc.
	 */
	cpu_startup();

	/*
	 * Initialize the debugger internals before enabling exceptions.
	 */
#ifdef DB
	db_init();
#endif

#if 0
	/*
	 * Turn on exception handlers.
	 */
	cpu_exception_init();
#endif

	/*
	 * Startup our physical page pool.
	 */
	page_init();

	/*
	 * Retrieve the amount of RAM available.  Skip where the kernel is
	 * loaded.
	 */
#define	KERNEL_MAX_SIZE		(4ul * 1024 * 1024)
#define	KERNEL_OFFSET		(1ul * 1024 * 1024)
#define	KERNEL_PHYSICAL_HOLE	(KERNEL_MAX_SIZE + KERNEL_OFFSET)
	if ((uintptr_t)_end >= KERNEL_PHYSICAL_HOLE)
		panic("%s: kernel end is beyond physical hole (%p >= %p).",
		      __func__, _end, (void *)KERNEL_PHYSICAL_HOLE);
	error = ofw_memory_init(0, KERNEL_PHYSICAL_HOLE);
	if (error != 0)
		panic("%s: ofw_memory_init failed: %m", __func__, error);

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
}

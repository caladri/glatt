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

vaddr_t
platform_start(register_t boot_args, register_t magic, register_t ofw_entry,
	       register_t argv, register_t argc)
{
	vaddr_t sp;
	int error;

	/*
	 * Initialize the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	platform_ofw_init(ofw_entry);

	ofw_init(macppc_ofw_call);

#ifdef DB
	db_init();
#endif

	startup_version();

	/*
	 * Startup our physical page pool.
	 */
	page_init();

	/*
	 * Add all global memory.  Processor-local memory will be added by
	 * the processor that owns it.  We skip the first 5MB of physical
	 * RAM because that's where the kernel will be loaded.  If we start
	 * to need more than 4MB, we're screwed.
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

#if 0
	/*
	 * Turn on exception handlers.
	 */
	cpu_exception_init();

	/*
	 * Set up data structures for later interrupt handling.
	 */
	cpu_interrupt_init();
#endif

	/*
	 * Early system startup.
	 */
	startup_init();

	/*
	 * Allocate a small stack.
	 */
	error = page_alloc_direct(&kernel_vm, PAGE_FLAG_DEFAULT, &sp);
	if (error != 0)
		panic("%s: page_alloc_direct failed: %m", __func__, error);
	return (sp + PAGE_SIZE);
}

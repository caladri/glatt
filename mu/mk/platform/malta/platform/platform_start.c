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
	/* XXX */
	for (;;) {
		/* NOTREACHED */
	}
}

void
platform_start(int32_t argc, int32_t argv, int32_t envp, uint32_t memsize)
{
	extern void malta_console_init(void);
	extern char __bss_start[], _end[];
	size_t membytes;
	int error;

	membytes = memsize;

	/*
	 * Initialize the BSS.
	 */
	memset(__bss_start, 0, _end - __bss_start);

	malta_console_init();

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
	 * the processor that owns it.  We skip the first 3MB of physical
	 * RAM because that's where the kernel will be loaded.  If we start
	 * to need more than 2MB, we're screwed.  Note that we have to load
	 * the kernel at 1MB to leave room for exception vectors and such
	 * at the start of physically-addressable memory.
	 */
#define	KERNEL_MAX_SIZE		(2ul * 1024 * 1024)
#define	KERNEL_OFFSET		(1ul * 1024 * 1024)
#define	KERNEL_PHYSICAL_HOLE	(KERNEL_MAX_SIZE + KERNEL_OFFSET)
	if (membytes <= KERNEL_PHYSICAL_HOLE)
		panic("%s: not enough attached memory.", __func__);
	membytes -= KERNEL_PHYSICAL_HOLE;
	error = page_insert_pages(KERNEL_PHYSICAL_HOLE, ADDR_TO_PAGE(membytes));
	if (error != 0)
		panic("page_insert_pages %lu..%lu failed: %m",
		      ADDR_TO_PAGE(KERNEL_PHYSICAL_HOLE),
		      ADDR_TO_PAGE(membytes), error);

	/*
	 * Turn on exception handlers.
	 */
	cpu_exception_init();

	/*
	 * Turn on the virtual memory subsystem.
	 */
	vm_init();

	pmap_bootstrap();
}

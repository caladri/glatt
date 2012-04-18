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
#include <core/console.h>
#include <platform/bootinfo.h>
#include <vm/vm_page.h>

extern char __bss_start[], __bss_stop[], _end[];

void
platform_halt(void)
{
	/* XXX */
	NOTREACHED();
}

void
platform_start(int argc, char **argv, int initcore, struct octeon_boot_descriptor *boot_descriptor)
{
	struct octeon_boot_memory_descriptor *boot_mem_descriptor;
	struct octeon_memory_descriptor *mem_descriptor;
	struct octeon_boot_info *boot_info;
	paddr_t mem_base, pcpu_addr;
	int error;

	/*
	 * Initialize the BSS.
	 */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	/*
	 * Set up the console.
	 */
	platform_console_init();

	/*
	 * Announce ourselves to the world.
	 */
	startup_version();

	/*
	 * Check the boot descriptor.
	 */
	if (boot_descriptor == NULL) {
		panic("No boot descriptor.");
	}
	if (boot_descriptor->size != sizeof *boot_descriptor) {
		panic("Boot descriptor has wrong size; got %u expected %zu.", boot_descriptor->size, sizeof *boot_descriptor);
	}
	if (boot_descriptor->info == 0) {
		panic("No boot info.");
	}

	/*
	 * Extract the boot info.
	 */
	boot_info = (struct octeon_boot_info *)XKPHYS_MAP(CCA_CNC, boot_descriptor->info);
	if (boot_info->memory_descriptor == 0) {
		panic("No memory descriptors.");
	}

	/*
	 * Extract memory for PCPU data.
	 */
	boot_mem_descriptor = (struct octeon_boot_memory_descriptor *)XKPHYS_MAP(CCA_CNC, boot_info->memory_descriptor);
	if (boot_mem_descriptor->head == 0) {
		panic("No memory descriptor head.");
	}

	mem_base = boot_mem_descriptor->head;
	pcpu_addr = 0;
	while (mem_base != 0) {
		mem_descriptor = (struct octeon_memory_descriptor *)XKPHYS_MAP(CCA_CNC, mem_base);

		if (mem_descriptor->size >= PAGE_SIZE) {
			pcpu_addr = mem_base + mem_descriptor->size - PAGE_SIZE;
			pcpu_addr = PAGE_FLOOR(pcpu_addr);
			if (pcpu_addr >= mem_base + sizeof *mem_descriptor) {
				mem_descriptor->size = pcpu_addr - mem_base;
				break;
			}
			pcpu_addr = 0;
		}

		mem_base = mem_descriptor->next;
	}
	if (pcpu_addr == 0) {
		panic("Could not steal page for PCPU data.");
	}

	/*
	 * Set up PCPU data, etc.  We'd like to do this earlier but can't yet.
	 */
	cpu_startup(pcpu_addr);

	/*
	 * Turn on exception handlers.
	 */
	cpu_exception_init();

	/*
	 * Startup our physical page pool.
	 */
	page_init();

	mem_base = boot_mem_descriptor->head;
	mem_descriptor = (struct octeon_memory_descriptor *)XKPHYS_MAP(CCA_CNC, mem_base);
	while (mem_base != 0) {
		paddr_t phys_addr = mem_base;
		size_t size = mem_descriptor->size;

		mem_base = mem_descriptor->next;
		mem_descriptor = (struct octeon_memory_descriptor *)XKPHYS_MAP(CCA_CNC, mem_base);

		if (!PAGE_ALIGNED(phys_addr)) {
			if (PAGE_SIZE - PAGE_OFFSET(phys_addr) >= size) {
				kcprintf("Skipping memory descriptor too small for page alignment.\n");
				continue;
			}
			size -= PAGE_SIZE - PAGE_OFFSET(phys_addr);
			phys_addr += PAGE_SIZE - PAGE_OFFSET(phys_addr);
		}

		if (size < PAGE_SIZE) {
			kcprintf("Skipping small memory descriptor.\n");
			continue;
		}

		error = page_insert_pages(phys_addr, size / PAGE_SIZE);
		if (error != 0)
			panic("page_insert_pages %lx..%lx failed: %m",
			      ADDR_TO_PAGE(phys_addr),
			      ADDR_TO_PAGE(size / PAGE_SIZE), error);
	}

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

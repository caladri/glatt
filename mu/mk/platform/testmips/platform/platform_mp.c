#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <cpu/cpuinfo.h>
#include <cpu/interrupt.h>
#include <cpu/memory.h>
#include <cpu/pcpu.h>
#include <cpu/tlb.h>
#include <db/db.h>
#include <io/device/console/console.h>
#include <io/device/device.h>
#include <io/device/driver.h>
#include <vm/page.h>
#include <vm/vm.h>

COMPILE_TIME_ASSERT(sizeof (struct pcpu) <= PAGE_SIZE);

#define	TEST_MP_DEV_BASE	0x11000000

#define	TEST_MP_DEV_WHOAMI	0x0000
#define	TEST_MP_DEV_NCPUS	0x0010
#define	TEST_MP_DEV_START	0x0020
#define	TEST_MP_DEV_STARTADDR	0x0030
#define	TEST_MP_DEV_STACK	0x0070
#define	TEST_MP_DEV_MEMORY	0x0090
#define	TEST_MP_DEV_IPI_ONE	0x00a0
#define	TEST_MP_DEV_IPI_MANY	0x00b0
#define	TEST_MP_DEV_IPI_READ	0x00c0

#define	TEST_MP_DEV_FUNCTION(f)						\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_MP_DEV_BASE + (f))
#define	TEST_MP_DEV_READ(f)						\
	(volatile uint64_t)*TEST_MP_DEV_FUNCTION(f)
#define	TEST_MP_DEV_WRITE(f, v)						\
	*TEST_MP_DEV_FUNCTION(f) = (v)

#define	TEST_MP_DEV_IPI_INTERRUPT	(4)

static struct device *platform_mp_bus;

static void platform_mp_attach(void);
static void platform_mp_attach_cpu(void);
static void platform_mp_ipi_interrupt(void *, int);
static void platform_mp_start_one(cpu_id_t, void (*)(void));

void
platform_mp_ipi_send(cpu_id_t cpu, enum ipi_type ipi)
{
	TEST_MP_DEV_WRITE(TEST_MP_DEV_IPI_ONE, (ipi << 16) | cpu);
}

void
platform_mp_ipi_send_but(cpu_id_t cpu, enum ipi_type ipi)
{
	TEST_MP_DEV_WRITE(TEST_MP_DEV_IPI_MANY, (ipi << 16) | cpu);
}

size_t
platform_mp_memory(void)
{
	return ((size_t)TEST_MP_DEV_READ(TEST_MP_DEV_MEMORY));
}

void
platform_mp_startup(void)
{
	/*
	 * Attach a device for the CPU if the bus is set up already.
	 */
	if (platform_mp_bus != NULL)
		platform_mp_attach_cpu();

	/* Install an IPI interrupt handler.  */
	cpu_hard_interrupt_establish(TEST_MP_DEV_IPI_INTERRUPT,
				     platform_mp_ipi_interrupt, NULL);
}

cpu_id_t
platform_mp_whoami(void)
{
	return ((cpu_id_t)TEST_MP_DEV_READ(TEST_MP_DEV_WHOAMI));
}

static void
platform_mp_ipi_interrupt(void *arg, int interrupt)
{
	uint64_t ipi;
	
	/*
	 * XXX
	 * Stab Anders.  The IPI source is lost.  We should encode the cpuid
	 * twice in the future.  Pity, it'd've been easy to store the whole
	 * idata in the pending queue, or something.
	 */
	while ((ipi = *TEST_MP_DEV_FUNCTION(TEST_MP_DEV_IPI_READ)) != IPI_NONE)
		mp_ipi_receive(ipi);
}

static void
platform_mp_start_all(void *arg)
{
	cpu_id_t cpu;
	uint64_t ncpus;

	platform_mp_attach();

	ncpus = TEST_MP_DEV_READ(TEST_MP_DEV_NCPUS);
	if (ncpus != 1) {
		kcprintf("cpu%u: multiprocessor system detected,"
			 " starting %lu processors.\n", mp_whoami(), ncpus);
		for (cpu = 0; cpu < ncpus; cpu++) {
			if (cpu == mp_whoami())
				platform_mp_attach_cpu();
			else
				platform_mp_start_one(cpu, platform_startup);
		}
		kcprintf("cpu%u: started processors.\n", mp_whoami());
	} else {
		kcprintf("cpu%u: uniprocessor system detected.\n", mp_whoami());
		platform_mp_attach_cpu();
	}
}
STARTUP_ITEM(platform_mp, STARTUP_MP, STARTUP_FIRST,
	     platform_mp_start_all, NULL);

static void
platform_mp_start_one(cpu_id_t cpu, void (*startup)(void))
{
	vaddr_t stack;
	int error;

	error = page_alloc_direct(&kernel_vm, &stack);
	if (error != 0)
		panic("%s: page_alloc_direct failed: %m", __func__, error);
	TEST_MP_DEV_WRITE(TEST_MP_DEV_STARTADDR, (uintptr_t)startup);
	TEST_MP_DEV_WRITE(TEST_MP_DEV_STACK, stack + PAGE_SIZE);
	TEST_MP_DEV_WRITE(TEST_MP_DEV_START, cpu);
}

static void
platform_mp_attach(void)
{
	struct driver *driver;
	int error;

	ASSERT(platform_mp_bus == NULL, "can only attach mp bus once.");

	driver = driver_lookup("mp");
	error = device_create(&platform_mp_bus, NULL, driver);
	if (error != 0)
		panic("%s: device create failed: %m", __func__, error);
}

static void
platform_mp_attach_cpu(void)
{
	struct device *device;
	struct driver *driver;
	int error;

	ASSERT(platform_mp_bus != NULL, "need mp bus to attach CPUs.");

	driver = driver_lookup("cpu");
	error = device_create(&device, platform_mp_bus, driver);
	if (error != 0)
		panic("%s: device create failed: %m", __func__, error);
}

DRIVER(mp, NULL, NULL);

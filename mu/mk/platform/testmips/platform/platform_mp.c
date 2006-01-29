#include <core/types.h>
#include <core/mp.h>
#include <core/startup.h>
#include <cpu/memory.h>
#include <io/device/console/console.h>

#define	TEST_MP_DEV_BASE	0x11000000

#define	TEST_MP_DEV_WHOAMI	0x0000
#define	TEST_MP_DEV_NCPUS	0x0010
#define	TEST_MP_DEV_START	0x0020
#define	TEST_MP_DEV_STARTADDR	0x0030
#define	TEST_MP_DEV_PAUSE	0x0050
#define	TEST_MP_DEV_UNPAUSE	0x0060
#define	TEST_MP_DEV_STACK	0x0070

#define	TEST_MP_DEV_FUNCTION(f)						\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_MP_DEV_BASE + (f))

static void platform_mp_start_one(void);

cpu_id_t
platform_mp_whoami(void)
{
	return (*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_WHOAMI));
}

int
platform_mp_block_but_one(cpu_id_t one)
{
	*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_PAUSE) = one;
	return (0);
}

int
platform_mp_unblock_but_one(cpu_id_t one)
{
	*TEST_MP_DEV_FUNCTION(TEST_MP_DEV_UNPAUSE) = one;
	return (0);
}

void
platform_mp_start_all(void)
{
	cpu_id_t cpu;
	uint64_t ncpus;

	ncpus = *TEST_MP_DEV_FUNCTION(TEST_MP_DEV_NCPUS);

	kcprintf("cpu%u: Starting %lu processors.\n", mp_whoami(), ncpus);
	for (cpu = 0; cpu < ncpus; cpu++) {
		if (cpu == mp_whoami())
			continue;
		/* XXX allocate new stack, set its address, start the CPU.  */
		kcprintf("cpu%u: skipping.\n", cpu);
	}
	kcprintf("cpu%u: Started processors, continuing.\n", mp_whoami());
	platform_mp_start_one();
}

static void
platform_mp_start_one(void)
{
	int error;

	error = mp_block_but_one(mp_whoami());
	if (error != 0) {
		/* XXX panic.  */
	}

	cpu_identify();

	error = mp_unblock_but_one(mp_whoami());
	if (error != 0) {
		/* XXX panic.  */
	}

	for (;;) {
		continue;
	}
}

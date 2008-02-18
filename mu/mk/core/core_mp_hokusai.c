#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>

static struct spinlock mp_hokusai_lock = SPINLOCK_INIT("HOKUSAI");
static bool mp_hokusai_ipi_registered;
static void (*mp_hokusai_callback)(void *);
static void *mp_hokusai_arg;

static volatile cpu_bitmask_t mp_hokusai_ready_bitmask;
static volatile cpu_bitmask_t mp_hokusai_done_bitmask;

static void mp_hokusai(void (*)(void *), void *);
static void mp_hokusai_clear(void);
static void mp_hokusai_enter(void);
static void mp_hokusai_exit(void);
static void mp_hokusai_ipi(void *, enum ipi_type);
static void mp_hokusai_ipi_send(void);

void
mp_hokusai_master(void (*masterf)(void *), void *masterp,
		  void (*slavef)(void *), void *slavep)
{
	ASSERT(mp_hokusai_ipi_registered, "Hokusai system must be ready.");

	spinlock_lock(&mp_hokusai_lock);

	mp_hokusai_callback = slavef;
	mp_hokusai_arg = slavep;

	mp_hokusai_clear();

	mp_hokusai_ipi_send();

	mp_hokusai(masterf, masterp);

	spinlock_unlock(&mp_hokusai_lock);
}

void
mp_hokusai_synchronize(void (*callback)(void *), void *p)
{
	mp_hokusai_master(callback, p, callback, p);
}

static void
mp_hokusai(void (*callback)(void *), void *p)
{
	mp_hokusai_enter();
	if (callback != NULL)
		callback(p);
	mp_hokusai_exit();
}

static void
mp_hokusai_clear(void)
{
	mp_hokusai_ready_bitmask = 0;
	mp_hokusai_done_bitmask = 0;
}

static void
mp_hokusai_enter(void)
{
	cpu_bitmask_t mask;

	mask = mp_cpu_running_mask();

	cpu_bitmask_set(&mp_hokusai_ready_bitmask, mp_whoami());

	while (mp_hokusai_ready_bitmask != mask)
		continue;
}

static void
mp_hokusai_exit(void)
{
	cpu_bitmask_t mask;

	/*
	 * If I'm the last hold-out, then don't spin.
	 */
	mask = mp_cpu_running_mask();

	cpu_bitmask_clear(&mask, mp_whoami());
	
	if (mp_hokusai_done_bitmask == mask) {
		cpu_bitmask_set(&mp_hokusai_done_bitmask, mp_whoami());
	} else {
		cpu_bitmask_set(&mp_hokusai_done_bitmask, mp_whoami());

		mask = mp_cpu_running_mask();

		while (mp_hokusai_done_bitmask != mask)
			continue;
	}
}

static void
mp_hokusai_ipi(void *arg, enum ipi_type ipi)
{
	mp_hokusai(mp_hokusai_callback, mp_hokusai_arg);
}

static void
mp_hokusai_ipi_send(void)
{
	mp_ipi_send_but(mp_whoami(), IPI_HOKUSAI);
}

static void
mp_hokusai_startup(void *arg)
{
	spinlock_lock(&mp_hokusai_lock);
	mp_ipi_register(IPI_HOKUSAI, mp_hokusai_ipi, NULL);

	mp_hokusai_ipi_registered = true;
	spinlock_unlock(&mp_hokusai_lock);
}
STARTUP_ITEM(hokusai, STARTUP_MP, STARTUP_FIRST, mp_hokusai_startup, NULL);

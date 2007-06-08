#include <core/types.h>
#include <core/mp.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <cpu/pcpu.h>
#include <db/db.h>
#include <io/device/console/console.h>

#define	PANIC_MAGIC	(0xcafebabe)

void
panic(const char *s, ...)
{
	static uint64_t magic;
	static struct spinlock console_lock = SPINLOCK_INIT("panic");
	va_list ap;
	int error;
	bool winner;

	winner = true;

	while (!atomic_compare_and_set_64(&magic, 0, PANIC_MAGIC)) {
		/*
		 * If it's currently PANIC_MAGIC then somebody has already
		 * panicked, handle one thing at a time.
		 */
		if (atomic_load_64(&magic) == PANIC_MAGIC) {
			winner = false;
			break;
		}
		ASSERT(false, "Should not be reached.");
	}

	if (winner) {
		/*
		 * Ask other CPUs to stop.
		 */
		mp_ipi_send_but(mp_whoami(), IPI_STOP);
	}

	spinlock_lock(&console_lock);
	kcprintf("panic: cpu%u: ", mp_whoami());
	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
	kcputs("\n");
	spinlock_unlock(&console_lock);

	if ((PCPU_GET(flags) & PCPU_FLAG_PANICKED) != 0) {
		kcprintf("cpu%u: double panic.\n", mp_whoami());
		mp_ipi_send(mp_whoami(), IPI_STOP);
	}
	PCPU_SET(flags, PCPU_GET(flags) | PCPU_FLAG_PANICKED);

	if (winner) {
		/*
		 * We get to go to the debugger!
		 */
		cpu_break();
	} else {
		spinlock_lock(&console_lock);
		kcprintf("cpu%u: secondary panic, halting.\n", mp_whoami());
		spinlock_unlock(&console_lock);
		/*
		 * Someone else is the primary panic, just stop ourselves.
		 * All we can do is log and let them go to the debugger.
		 */
		cpu_halt();
	}
}

#if 0
static void
db_startup_panic(void *arg)
{
	panic("%s: panicking at startup because you want me to", __func__);
}
STARTUP_ITEM(panic, STARTUP_MAIN, STARTUP_BEFORE, db_startup_panic, NULL);
#endif

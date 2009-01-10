#include <core/types.h>
#include <core/critical.h>
#include <core/shlock.h>
#include <core/mp.h>
#include <cpu/atomic.h>

void
shlock_init(struct shlock *shl, const char *name)
{
	shl->shl_name = name;
	shl->shl_sharecnt = 0;
	shl->shl_xowner = CPU_ID_INVALID;
}

void
shlock_slock(struct shlock *shl)
{
	critical_enter();
	while (!atomic_cmpset64(&shl->shl_xowner, CPU_ID_INVALID,
				mp_whoami())) {
		if (atomic_load64(&shl->shl_xowner) == mp_whoami())
			panic("%s: refusing to recurse on %s", __func__,
			      shl->shl_name);
		critical_exit();
		critical_enter();
	}
	atomic_increment64(&shl->shl_sharecnt);
	atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
	critical_exit();
}

void
shlock_sunlock(struct shlock *shl)
{
	atomic_decrement64(&shl->shl_sharecnt);
}

void
shlock_xlock(struct shlock *shl)
{
	for (;;) {
		critical_enter();
		while (!atomic_cmpset64(&shl->shl_xowner, CPU_ID_INVALID,
					mp_whoami())) {
			if (atomic_load64(&shl->shl_xowner) == mp_whoami()) {
				panic("%s: refusing to recurse on %s", __func__,
				      shl->shl_name);
			}

			critical_exit();

			/* Leave critical section to allow for interrupts.  */

			critical_enter();
		}

		if (atomic_load64(&shl->shl_sharecnt) == 0)
			break;
		atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
	}
}

void
shlock_xunlock(struct shlock *shl)
{
	atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
	critical_exit();
}

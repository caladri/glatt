#include <core/types.h>
#include <core/shlock.h>
#include <core/mp.h>
#include <cpu/atomic.h>

void
shlock_init(struct shlock *shl, const char *name)
{
	shl->shl_name = name;
	atomic_store64(&shl->shl_sharecnt, 0);
	atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
}

void
shlock_slock(struct shlock *shl)
{
	critical_section_t crit;

	crit = critical_enter();
	while (!atomic_cmpset64(&shl->shl_xowner, CPU_ID_INVALID,
				mp_whoami())) {
		if (atomic_load64(&shl->shl_xowner) == mp_whoami())
			panic("%s: refusing to recurse on %s", __func__,
			      shl->shl_name);
		crit = critical_enter();
		critical_exit(crit);
	}
	atomic_increment64(&shl->shl_sharecnt);
	atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
	critical_exit(crit);
}

void
shlock_sunlock(struct shlock *shl)
{
	atomic_decrement64(&shl->shl_sharecnt);
}

void
shlock_xlock(struct shlock *shl)
{
	critical_section_t crit;

	for (;;) {
		crit = critical_enter();
		while (!atomic_cmpset64(&shl->shl_xowner, CPU_ID_INVALID,
					mp_whoami())) {
			if (atomic_load64(&shl->shl_xowner) == mp_whoami()) {
				panic("%s: refusing to recurse on %s", __func__,
				      shl->shl_name);
			}

			critical_exit(crit);

			/* Leave critical section to allow for interrupts.  */

			crit = critical_enter();
		}

		if (atomic_load64(&shl->shl_sharecnt) == 0)
			break;
		atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
	}
	shl->shl_xcrit = crit;
}

void
shlock_xunlock(struct shlock *shl)
{
	critical_section_t crit;

	crit = shl->shl_xcrit;
	atomic_store64(&shl->shl_xowner, CPU_ID_INVALID);
	critical_exit(crit);
}

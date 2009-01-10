#ifndef	_CORE_SHLOCK_H_
#define	_CORE_SHLOCK_H_

/*
 * Reader-writer/shared-exclusive spin-locks for short-use shared locking.
 * Doesn't support upgrading, since that requires adding a try_lock to spinlock,
 * and we don't want that.  (Yet?)
 *
 * Shared locks exclude exclusive locks, but do not enter a critical section.
 * Exclusive locks exclude shared and exclusive locks and enter a critical
 * section.
 */

struct shlock {
	const char *shl_name;
	uint64_t shl_sharecnt;
	uint64_t shl_xowner;
};

void shlock_init(struct shlock *, const char *);
void shlock_slock(struct shlock *);
void shlock_sunlock(struct shlock *);
void shlock_xlock(struct shlock *);
void shlock_xunlock(struct shlock *);

#endif /* !_CORE_SHLOCK_H_ */

#ifndef	_CPU_ATOMIC_H_
#define	_CPU_ATOMIC_H_

	/* Dummy atomic functions!  */

static inline uint64_t
atomic_load_64(volatile uint64_t *p)
{
	return (*p);
}

static inline void
atomic_store_64(volatile uint64_t *p, uint64_t v)
{
	*p = v;
}

static inline bool
atomic_compare_and_set_64(volatile uint64_t *p, uint64_t o, uint64_t v)
{
	if (atomic_load_64(p) == o) {
		atomic_store_64(p, v);
		return (true);
	}
	return (false);
}

static inline void
atomic_decrement_64(volatile uint64_t *p)
{
	uint64_t o;
	o = atomic_load_64(p);
	while (!atomic_compare_and_set_64(p, o, o - 1))
		o = atomic_load_64(p);
}

static inline void
atomic_increment_64(volatile uint64_t *p)
{
	uint64_t o;
	o = atomic_load_64(p);
	while (!atomic_compare_and_set_64(p, o, o + 1))
		o = atomic_load_64(p);
}

#endif /* !_CPU_ATOMIC_H_ */

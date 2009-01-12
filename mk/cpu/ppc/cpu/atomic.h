#ifndef	_CPU_ATOMIC_H_
#define	_CPU_ATOMIC_H_

	/* Dummy atomic functions!  */

static inline uint64_t
atomic_load64(const volatile uint64_t *p)
{
	return (*p);
}

static inline void
atomic_store64(volatile uint64_t *p, uint64_t v)
{
	*p = v;
}

static inline bool
atomic_cmpset64(volatile uint64_t *p, uint64_t o, uint64_t v)
{
	if (*p != o)
		return (false);
	*p = v;
	return (true);
}

static inline void
atomic_decrement64(volatile uint64_t *p)
{
	*p = *p - 1;
}

static inline void
atomic_increment64(volatile uint64_t *p)
{
	*p = *p + 1;
}

static inline void
atomic_clear64(volatile uint64_t *p, uint64_t mask)
{
	*p = *p & ~mask;
}

static inline void
atomic_set64(volatile uint64_t *p, uint64_t mask)
{
	*p = *p | mask;
}

#endif /* !_CPU_ATOMIC_H_ */

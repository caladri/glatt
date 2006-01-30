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
	uint64_t temp;
	int res;

	__asm __volatile (
	"1:\n\t"
	"move	%[res], $0\n\t"
	"lld	%[temp], %[p]\n\t"
	"bne	%[temp], %[o], 2f\n\t"
	"move	%[temp], %[v]\n\t"
	"li	%[res], 1\n\t"
	"scd	%[temp], %[p]\n\t"
	"beqz	%[temp], 1b\n\t"
	"2:\n\t"
	: [res] "=&r"(res), [temp] "=&r"(temp), [p] "+m"(*p)
	: [o] "r"(o), [v] "r"(v)
	: "memory"
	);

	return (res != 0);
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

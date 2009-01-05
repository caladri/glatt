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
	uint64_t temp;
	int res;

	asm volatile (
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
atomic_decrement64(volatile uint64_t *p)
{
	uint64_t temp;

	asm volatile (
	"1:\n\t"
	"lld	%[temp], %[p]\n\t"
	"dsubu	%[temp], 1\n\t"
	"scd	%[temp], %[p]\n\t"
	"beqz	%[temp], 1b\n\t"
	: [temp] "=&r"(temp), [p] "+m"(*p)
	:
	: "memory"
	);
}

static inline void
atomic_increment64(volatile uint64_t *p)
{
	uint64_t temp;

	asm volatile (
	"1:\n\t"
	"lld	%[temp], %[p]\n\t"
	"daddu	%[temp], 1\n\t"
	"scd	%[temp], %[p]\n\t"
	"beqz	%[temp], 1b\n\t"
	: [temp] "=&r"(temp), [p] "+m"(*p)
	:
	: "memory"
	);
}

static inline void
atomic_mask64(volatile uint64_t *p, uint64_t mask)
{
	uint64_t temp;

	asm volatile (
	"1:\n\t"
	"lld	%[temp], %[p]\n\t"
	"and	%[temp], %[mask]\n\t"
	"scd	%[temp], %[p]\n\t"
	"beqz	%[temp], 1b\n\t"
	: [temp] "=&r"(temp), [p] "+m"(*p)
	: [mask] "r"(mask)
	: "memory"
	);
}

static inline void
atomic_clear64(volatile uint64_t *p, uint64_t mask)
{
	atomic_mask64(p, ~mask);
}

static inline void
atomic_set64(volatile uint64_t *p, uint64_t mask)
{
	uint64_t temp;

	asm volatile (
	"1:\n\t"
	"lld	%[temp], %[p]\n\t"
	"or	%[temp], %[mask]\n\t"
	"scd	%[temp], %[p]\n\t"
	"beqz	%[temp], 1b\n\t"
	: [temp] "=&r"(temp), [p] "+m"(*p)
	: [mask] "r"(mask)
	: "memory"
	);
}

#endif /* !_CPU_ATOMIC_H_ */

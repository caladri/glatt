#include <core/types.h>

uint64_t __udivdi3(uint64_t, uint64_t);
uint64_t __umoddi3(uint64_t, uint64_t);
uint64_t qdivrem(uint64_t, uint64_t, uint64_t *);

uint64_t
__udivdi3(uint64_t n, uint64_t d)
{
	uint64_t q;

	q = qdivrem(n, d, NULL);

	return (q);
}

uint64_t
__umoddi3(uint64_t n, uint64_t d)
{
	uint64_t r;

	(void)qdivrem(n, d, &r);

	return (r);
}

uint64_t
qdivrem(uint64_t n, uint64_t d, uint64_t *rp)
{
	uint64_t q, r;

	if (d == 0)
		panic("%s: divide by zero.", __func__);

	q = 0;
	r = 0;

	while (n >= d) {
		q++;
		n -= d;
	}
	r = n;

	if (rp != NULL)
		*rp = r;
	return (q);
}

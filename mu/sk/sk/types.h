#ifndef	_SK_TYPES_H_
#define	_SK_TYPES_H_

#include <platform/sk/types.h>

/*
 * Standard types.
 */
typedef enum {
	true	= 1 == 1,
	false	= !true
} bool;

typedef	uint64_t	size_t;
typedef	int64_t		ssize_t;

/*
 * Standard definitions.
 */
#define	NULL	((void *)(uintptr_t)0)

/*
 * Macro helpers.
 */
#define	MACRO_BLOCK_BEGIN	do
#define	MACRO_BLOCK_END		while (0)

#define	_CONCAT(x, y)		x ## y
#define	CONCAT(x, y)		_CONCAT(x, y)

/*
 * Bitmask helpers.
 */
#define	POPCNT(x)							\
	(((PX_(x) + (PX_(x) >> 4)) & 0x0F0F0F0F) % 255)
#define	PX_(x)								\
	((x) - (((x) >> 1) & 0x77777777)				\
	     - (((x) >> 2) & 0x33333333)				\
	     - (((x) >> 3) & 0x11111111))
#define	LOG2(x)								\
	(POPCNT((x) - 1))

/*
 * Compile-time assertions.
 */
#define COMPILE_TIME_ASSERT(p)						\
	typedef int CONCAT(ctassert_, __LINE__) [(!(p) * -1) + (p)]

#endif /* !_SK_TYPES_H_ */

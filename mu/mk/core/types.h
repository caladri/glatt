#ifndef	_CORE_TYPES_H_
#define	_CORE_TYPES_H_

#define	_IN_CORE_TYPES_H_

#include <core/_stdarg.h>
#include <cpu/types.h>

	/* Special pointers.  */
#define	NULL		((void *)0x0)

	/* Size types and friends.  */

typedef	uint64_t	size_t;
typedef	int64_t		ssize_t;

	/* C99 boolean support.  */

typedef	enum _bool {
	false =		0 != 0,
	true =		1 == 1,
} bool;

	/* Pull in helper macros.  */

#include <core/_macro.h>

#undef _IN_CORE_TYPES_H_

#endif /* !_CORE_TYPES_H_ */

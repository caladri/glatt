#ifndef	_CORE_TYPES_H_
#define	_CORE_TYPES_H_

#include <core/_stdarg.h>
#include <cpu/types.h>

	/* Size types and friends.  */

typedef	uint64_t	size_t;
typedef	int64_t		ssize_t;

	/* C99 boolean support.  */

typedef	enum _bool {
	false =		0 != 0,
	true =		1 == 1,
} bool;

#endif /* !_CORE_TYPES_H_ */

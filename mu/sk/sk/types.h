#ifndef	_SK_TYPES_H_
#define	_SK_TYPES_H_

#include <cpu/sk/types.h>

typedef enum {
	true	= 1 == 1,
	false	= !true
} bool;

typedef	uint64_t	size_t;
typedef	int64_t		ssize_t;

/*
 * Macro helpers.
 */
#define	MACRO_BLOCK_BEGIN	do
#define	MACRO_BLOCK_END		while (0)

#endif /* !_SK_TYPES_H_ */

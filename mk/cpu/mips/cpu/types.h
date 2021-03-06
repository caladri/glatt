#ifndef	_CPU_TYPES_H_
#define	_CPU_TYPES_H_

	/* C99 integer types with bit-width specification.  */

typedef	signed char		int8_t;
typedef	unsigned char		uint8_t;
typedef	signed short		int16_t;
typedef	unsigned short		uint16_t;
typedef	signed int		int32_t;
typedef	unsigned int		uint32_t;
typedef	signed long		int64_t;
typedef	unsigned long		uint64_t;

	/* Pointer-related integer types.  */

typedef	int64_t			intptr_t;
typedef	uint64_t		uintptr_t;

	/* C99 integer types otherwise.  */

typedef	signed long		intmax_t;
typedef	unsigned long		uintmax_t;

	/* Types just for mk.  */
#ifdef MK

	/* Virtual memory subsystem.  */

typedef	uint64_t	paddr_t;
typedef	uint64_t	vaddr_t;

#include <platform/types.h>
#endif

#endif /* !_CPU_TYPES_H_ */

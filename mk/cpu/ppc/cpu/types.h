#ifndef	_CPU_TYPES_H_
#define	_CPU_TYPES_H_

	/* C99 integer types with bit-width specification.  */

typedef	signed char		int8_t;
typedef	unsigned char		uint8_t;
typedef	signed short		int16_t;
typedef	unsigned short		uint16_t;
typedef	signed int		int32_t;
typedef	unsigned int		uint32_t;
typedef	signed long long	int64_t;
typedef	unsigned long long	uint64_t;

	/* Pointer-related integer types.  */

typedef	int32_t			intptr_t;
typedef	uint32_t		uintptr_t;

	/* C99 integer types otherwise.  */

typedef	signed long long	intmax_t;
typedef	unsigned long long	uintmax_t;

	/* Virtual memory subsystem.  */

typedef	uint32_t	paddr_t;
typedef	uint32_t	vaddr_t;

#include <platform/types.h>

#endif /* !_CPU_TYPES_H_ */

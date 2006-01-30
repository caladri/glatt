#include <core/_macro.h>
#include <cpu/asm.h>

	/* Vectors for exception, interrupt and TLB miss handling.  */

#define	VECTOR_ENTRY(v)							\
	ENTRY(CONCAT(v, _vector))

#define	VECTOR_END(v)							\
	END(CONCAT(v, _vector));					\
	SYMBOL(CONCAT(v, _vector_end))

	NOREORDER

VECTOR_ENTRY(exception)
VECTOR_END(exception)

VECTOR_ENTRY(xtlb)
VECTOR_END(xtlb)

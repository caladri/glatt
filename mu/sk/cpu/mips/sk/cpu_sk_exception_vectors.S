#include <cpu/sk/asm.h>
#include <cpu/sk/exception.h>

	NOREORDER

	.text

VECTOR_ENTRY(Generic)
1:	nop
	b	1b
	nop
VECTOR_END(Generic)

#include <cpu/asm.h>

	NOREORDER

	.data
	.space 1024
SYMBOL(tempstack)

	.text
ENTRY(main)

	/* Clear cause register.  */
	mtc0	zero, CP0_CAUSE

	/* Switch to a temporary stack.  */
	dla	sp, tempstack

	/* Set up the GP.  */
	dla	gp, _gp
END(main)

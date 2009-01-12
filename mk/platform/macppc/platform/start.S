#include <cpu/asm.h>

	.data
	.space 1024
SYMBOL(tempstack)

	.text
	/*
	 * r3 = boot_args
	 * r4 = magic
	 * r5 = ofw
	 * r6, r7 = args
	 */
ENTRY(start)
	lis	%r1, tempstack@ha
	ori	%r1, %r1, tempstack@l

	/* XXX Save OFW SPRG[0..3].  */

	bl	platform_start
END(start)

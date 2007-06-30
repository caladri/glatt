#ifndef	_CPU_SK_ASM_H_
#define	_CPU_SK_ASM_H_

#include <cpu/sk/register.h>

#ifndef	ASSEMBLER
#error "Cannot include <cpu/asm.h> from C."
#endif /* ASSEMBLER */

#define	SYMBOL(s)	.globl s; s:

#define	ENTRY(s)	.p2align 4; .globl s; .ent s; s:
#define	END(s)		.end s

#define	NOREORDER	.set noreorder

#endif /* !_CPU_SK_ASM_H_ */

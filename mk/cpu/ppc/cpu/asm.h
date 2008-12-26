#ifndef	_CPU_ASM_H_
#define	_CPU_ASM_H_

#include <cpu/register.h>

#ifndef	ASSEMBLER
#error "Cannot include <cpu/asm.h> from C."
#endif /* ASSEMBLER */

#define	SYMBOL(s)	.globl s; s:

#define	ENTRY(s)	.p2align 4; .globl s; s:
#define	END(s)		/* nothing */

#define	NOREORDER	.set noreorder

#endif /* !_CPU_ASM_H_ */

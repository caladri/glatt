#ifndef _BUILD__ASSYM_H_
#define	_BUILD__ASSYM_H_

/*
 * XXX GCC is broken and won't allow constraints in global assembly, even when
 * those constraints are, say, "n", constants which do not require any
 * computation.  I'm astounded --juli.
 */

#define	DEFINE(name, value)						\
void gcc_is_dumb_ ## name(void);					\
void gcc_is_dumb_ ## name(void)						\
{									\
	__asm ("@assym@ define " #name " %0" : : "n"(value));		\
} struct __hack

#define	DEFINE_CONSTANT(name)						     \
void gcc_is_dumb_ ## name(void);					\
void gcc_is_dumb_ ## name(void)						\
{									\
	__asm ("@assym@ define " #name " %0" : : "n"(name));		\
} struct __hack

#endif /* !_BUILD__ASSYM_H_ */

#ifndef	_CORE__MACRO_H_
#define	_CORE__MACRO_H_

#ifndef	_IN_CORE_TYPES_H_
#ifndef	ASSEMBLER
#error "Do not directly include <core/_macro.h> from C."
#endif
#endif

	/* Handy macros.  */

#define	MIN(a, b)		((a) < (b) ? (a) : (b))
#define	MAX(a, b)		((a) > (b) ? (a) : (b))

	/* Preprocessor magic.  */

#define	_CONCAT(x, y)		x ## y
#define	CONCAT(x, y)		_CONCAT(x, y)
#define	STRING(t)		#t

	/* Function, variable, etc., attributes.  */

#define	__noreturn		__attribute__ ((__noreturn__))
#define	__packed		__attribute__ ((__packed__))
#define	__unused		__attribute__ ((__unused__))

	/* Compile-time assertions.  */

#ifndef	ASSEMBLER
#define	COMPILE_TIME_ASSERT(p)						\
	typedef	uint8_t CONCAT(ctassert_, __LINE__) [(!(p) * -1) + (p)]

COMPILE_TIME_ASSERT(true);
COMPILE_TIME_ASSERT(!false);
#endif

#endif /* !_CORE__MACRO_H_ */

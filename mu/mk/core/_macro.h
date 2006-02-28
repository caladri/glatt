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

#define	offsetof(s, f)		((size_t)((uintptr_t)(&(((s *)NULL)->f))))

	/* Preprocessor magic.  */

#define	_CONCAT(x, y)		x ## y
#define	CONCAT(x, y)		_CONCAT(x, y)
#define	STRING(t)		#t

	/* Function, variable, etc., attributes.  */

#define	__noreturn		__attribute__ ((__noreturn__))
#define	__packed		__attribute__ ((__packed__))
#define	__printf(f, va)		__attribute__ ((__format__ (__printf__, f, va)))
#define	__section(s)		__attribute__ ((__section__ (s)))
#define	__unused		__attribute__ ((__unused__))
#define	__used			__attribute__ ((__used__))

	/* Compile-time assertions.  */

#ifndef	ASSEMBLER
#define	COMPILE_TIME_ASSERT(p)						\
	typedef	uint8_t CONCAT(ctassert_, __LINE__) [(!(p) * -1) + (p)]

COMPILE_TIME_ASSERT(true);
COMPILE_TIME_ASSERT(!false);
#endif

	/* Using separate sections to implement dynamic lists.  */
#define	SECTION_START(s)	_CONCAT(__start_, s)
#define	SECTION_STOP(s)		_CONCAT(__stop_, s)

#define	SET(set, type)							\
	extern type *SECTION_START(_CONCAT(set_, set));			\
	extern type *SECTION_STOP(_CONCAT(set_, set))

#define	SET_ADD(set, v)							\
	static void const *const _set_ ## set ## _ ## v			\
		__section("set_" #set) __used = &(v)

#define	SET_BEGIN(set)							\
	(&SECTION_START(_CONCAT(set_, set)))

#define	SET_END(set)							\
	(&SECTION_STOP(_CONCAT(set_, set)))

#endif /* !_CORE__MACRO_H_ */

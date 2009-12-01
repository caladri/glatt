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

#define	ROUNDUP(x, y)		((((x) + ((y) - 1)) / (y)) * y)

#define	offsetof(s, f)		((size_t)((uintptr_t)(&(((s *)NULL)->f))))

	/* Preprocessor magic.  */

#define	_CONCAT(x, y)		x ## y
#define	CONCAT(x, y)		_CONCAT(x, y)
#define	_STRING(t)		#t
#define	STRING(t)		_STRING(t)

	/* Function, variable, etc., attributes.  */

#define	__noreturn		__attribute__ ((__noreturn__))
#define	__packed		__attribute__ ((__packed__))
#define	__printf(f, va)		__attribute__ ((__format__ (__printf__, f, va)))
#define	__section(s)		__attribute__ ((__section__ (s)))
#define	__unused		__attribute__ ((__unused__))
#define	__used			__attribute__ ((__used__))
#define	__malloc		__attribute__ ((__malloc__))
#if !defined(ASSEMBLER) /* XXX GCC doesn't respect -std=c99 for preprocessing assembly.  */
#define	__non_null(...)		__attribute__ ((__nonnull__ (__VA_ARGS__)))
#endif

	/* Bitmask helpers.  */

#define	POPCNT(x)							\
	(((PX_(x) + (PX_(x) >> 4)) & 0x0F0F0F0F) % 255)
#define	PX_(x)								\
	((x) - (((x) >> 1) & 0x77777777)				\
	     - (((x) >> 2) & 0x33333333)				\
	     - (((x) >> 3) & 0x11111111))
#define	LOG2(x)								\
	(POPCNT((x) - 1))

#define	ISPOW2(x)							\
	(((x) & ((x) - 1)) == 0)

	/* Compile-time assertions.  (Only use in source files.)  */

#define	COMPILE_TIME_ASSERT(p)					\
	typedef	uint8_t CONCAT(ctassert_, __LINE__) [(!(p) * -1) + (p)]

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

#define	SET_FOREACH(v, set)						\
	for ((v) = SET_BEGIN(set); (v) != SET_END(set); (v)++)

#endif /* !_CORE__MACRO_H_ */

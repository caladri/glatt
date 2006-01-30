#ifndef	_CORE_MACRO_H_
#define	_CORE_MACRO_H_

#define	MIN(a, b)		((a) < (b) ? (a) : (b))
#define	MAX(a, b)		((a) > (b) ? (a) : (b))

#define	_CONCAT(x, y)		x ## y
#define	CONCAT(x, y)		_CONCAT(x, y)
#define	STRING(t)		#t

#define	COMPILE_TIME_ASSERT(p)						\
	typedef	uint8_t CONCAT(ctassert_, __LINE__) [!(p) * -1]

COMPILE_TIME_ASSERT(true);
COMPILE_TIME_ASSERT(!false);

#endif /* !_CORE_MACRO_H_ */

#ifndef	_CORE__STDARG_H_
#define	_CORE__STDARG_H_

#ifndef	_IN_CORE_TYPES_H_
#error "Do not include <core/_stdarg.h> directly."
#endif

typedef	__builtin_va_list	va_list;

#define	va_start(ap, l)		__builtin_va_start((ap), (l))
#define	va_arg(ap, t)		__builtin_va_arg((ap), t)
#define	va_end(ap)		__builtin_va_end((ap))

#endif /* !_CORE__STDARG_H_ */

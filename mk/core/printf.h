#ifndef	_CORE_PRINTF_H_
#define	_CORE_PRINTF_H_

void kfvprintf(void (*)(void *, char), void (*)(void *, const char *, size_t),
	       void *, const char *, va_list) __non_null(1, 2, 4);

#endif /* !_CORE_PRINTF_H_ */

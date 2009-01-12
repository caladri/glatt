#ifndef	_CORE_PRINTF_H_
#define	_CORE_PRINTF_H_

void kfvprintf(void (*)(void *, char), void (*)(void *, const char *, size_t),
	       void *, const char *, va_list);

#endif /* !_CORE_PRINTF_H_ */

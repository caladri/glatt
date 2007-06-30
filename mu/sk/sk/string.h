#ifndef	_SK_STRING_H_
#define	_SK_STRING_H_

static inline void
memset(void *data, int fill, size_t length)
{
	char *p = data;
	while (length--)
		*p++ = fill;
}

#endif /* !_SK_STRING_H_ */

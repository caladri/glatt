#ifndef	_CORE_STRING_H_
#define	_CORE_STRING_H_

static inline void
memcpy(void *dst, const void *src, size_t len)
{
	char *d;
	const char *s;

	d = dst;
	s = src;

	while (len--)
		*d++ = *s++;
}

static inline void
memset(void *dst, int val, size_t len)
{
	char *d = dst;

	while (len--)
		*d++ = val;
}

static inline const char *
strchr(const char *str, char ch)
{
	const char *p;

	for (p = str; *p != '\0'; p++) {
		if (*p != ch)
			continue;
		return (p);
	}
	return (NULL);
}

static inline int
strcmp(const char *a, const char *b)
{
	while (*a != '\0' && *a == *b) {
		a++;
		b++;
	}
	return (*a - *b);
}

static inline size_t
strlcpy(char *dst, const char *src, size_t len)
{
	const char *s = src++;

	if (len == 0)
		goto count;
	while (--len)
		if ((*dst++ = *s++) == '\0')
			return (s - src);
	*dst = '\0';
count:	while (*s++ != '\0')
		continue;
	return (s - src);
}

static inline size_t
strlen(const char *s)
{
	const char *p;

	for (p = s; *p != '\0'; p++)
		continue;
	return (p - s);
}

static inline int
strncmp(const char *a, const char *b, size_t n)
{
	while (n-- != 0 && *a != '\0' && *a == *b) {
		if (n == 0)
			return (0);
		a++;
		b++;
	}
	return (*a - *b);
}

#endif /* !_CORE_STRING_H_ */

#ifndef	_CORE_STRING_H_
#define	_CORE_STRING_H_

/*
 * Need declarations in order to set attributes.
 */
static inline void memcpy(void *, const void *, size_t) __non_null(1, 2);
static inline void memset(void *, int, size_t) __non_null(1);
static inline const char *strchr(const char *, char) __non_null(1);
static inline int strcmp(const char *, const char *) __non_null(1, 2);
static inline size_t strlcpy(char *, const char *, size_t) __non_null(1, 2);
static inline size_t strlen(const char *) __non_null(1);
static inline int strncmp(const char *, const char *, size_t) __non_null(1, 2);

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

void snprintf(char *, size_t, const char *, ...) __non_null(1, 3);
void vsnprintf(char *, size_t, const char *, va_list) __non_null(1, 3);

#endif /* !_CORE_STRING_H_ */

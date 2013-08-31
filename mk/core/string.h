#ifndef	_CORE_STRING_H_
#define	_CORE_STRING_H_

/*
 * Need declarations in order to set attributes.
 */
static inline void memcpy(void *, const void *, size_t) __non_null(1, 2);
static inline void memset(void *, int, size_t) __non_null(1);
static inline const char *strchr(const char *, char) __non_null(1) __check_result;
static inline int strcmp(const char *, const char *) __non_null(1, 2) __check_result;
static inline size_t strlcpy(char *, const char *, size_t) __non_null(1, 2);
static inline size_t strlcat(char *, const char *, size_t) __non_null(1, 2);
static inline size_t strlen(const char *) __non_null(1) __check_result;
static inline int strncmp(const char *, const char *, size_t) __non_null(1, 2) __check_result;

static inline void
memcpy(void *dst, const void *src, size_t len)
{
	uint8_t *d;
	const uint8_t *s;

	if ((((uintptr_t)dst | (uintptr_t)src | len) & (sizeof (uint64_t) - 1)) == 0) {
		uint64_t *d64 = dst;
		const uint64_t *s64 = src;

		len /= sizeof (uint64_t);
		while (len--)
			*d64++ = *s64++;
		return;
	}

	if ((((uintptr_t)dst | (uintptr_t)src | len) & (sizeof (uint32_t) - 1)) == 0) {
		uint32_t *d32 = dst;
		const uint32_t *s32 = src;

		len /= sizeof (uint32_t);
		while (len--)
			*d32++ = *s32++;
		return;
	}

	if ((((uintptr_t)dst | (uintptr_t)src | len) & (sizeof (uint16_t) - 1)) == 0) {
		uint16_t *d16 = dst;
		const uint16_t *s16 = src;

		len /= sizeof (uint16_t);
		while (len--)
			*d16++ = *s16++;
		return;
	}

	d = dst;
	s = src;

	while (len--)
		*d++ = *s++;
}

static inline void
memset(void *dst, int val, size_t len)
{
	char *d = dst;

	val &= 0xff;

	if ((((uintptr_t)dst | len) & (sizeof (int64_t) - 1)) == 0) {
		int64_t *d64 = dst;

		len /= sizeof (int64_t);
		while (len--)
			*d64++ =
				((uint64_t)val << 0) |
				((uint64_t)val << 8) |
				((uint64_t)val << 16) |
				((uint64_t)val << 24) |
				((uint64_t)val << 32) |
				((uint64_t)val << 40) |
				((uint64_t)val << 48) |
				((uint64_t)val << 56) |
				((uint64_t)val << 56);
		return;
	}

	if ((((uintptr_t)dst | len) & (sizeof (int32_t) - 1)) == 0) {
		int32_t *d32 = dst;

		len /= sizeof (int32_t);
		while (len--)
			*d32++ =
				((uint32_t)val << 0) |
				((uint32_t)val << 8) |
				((uint32_t)val << 16) |
				((uint32_t)val << 24);
		return;
	}

	if ((((uintptr_t)dst | len) & (sizeof (int16_t) - 1)) == 0) {
		int16_t *d16 = dst;

		len /= sizeof (int16_t);
		while (len--)
			*d16++ =
				((uint32_t)val << 0) |
				((uint32_t)val << 8);
		return;
	}

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

/*
 * XXX
 * Half-assed.  Do some real testing.
 */
static inline size_t
strlcat(char *dst, const char *src, size_t len)
{
	size_t dlen;

	dlen = strlen(dst);
	return (dlen + strlcpy(dst + dlen, src, len - dlen));
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

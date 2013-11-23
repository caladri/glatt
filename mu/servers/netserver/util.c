#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>

#include <libmu/common.h>

#include "util.h"

void
format_ip(char *buf, size_t buflen, uint32_t ip)
{
	snprintf(buf, buflen, "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
}

int
parse_ip(const char *s, uint32_t *ip)
{
	unsigned octet;

	for (octet = 4; octet != 0; octet--) {
		uint8_t v;

		v = 0;
		while (*s != '.' && *s != '\0') {
			uint8_t d;
			if (*s < '0' || *s > '9')
				return (ERROR_INVALID);
			d = *s - '0';
			if ((256 - (v * 10)) <= d)
				return (ERROR_INVALID);
			v = (v * 10) + d;
			s++;
		}
		*ip &= ~(0xff << ((octet - 1) * 8));
		if (v != 0)
			*ip |= v << ((octet - 1) * 8);
		if (*s == '\0')
			break;
		s++;
	}
	if (*s != '\0')
		return (ERROR_INVALID);
	return (0);
}

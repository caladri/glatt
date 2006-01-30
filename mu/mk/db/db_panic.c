#include <core/types.h>
#include <db/db.h>
#include <io/device/console/console.h>

void
panic(const char *s, ...)
{
	va_list ap;

	kcputs("panic: ");
	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
	kcputs("\n");
	cpu_break();
}

#include <core/types.h>
#include <core/mp.h>
#include <db/db.h>
#include <io/device/console/console.h>

void
panic(const char *s, ...)
{
	va_list ap;
	int error;

#if 0
	error = mp_block_but_one(mp_whoami());
	if (error != 0) {
		cpu_break();
		//panic("%s: can't block others: %u", __func__, error);
	}
#endif

	kcputs("panic: ");
	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
	kcputs("\n");
	cpu_break();
}

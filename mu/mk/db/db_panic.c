#include <core/types.h>
#include <core/mp.h>
#include <db/db.h>
#include <io/device/console/console.h>

void
panic(const char *s, ...)
{
	va_list ap;
	int error;

	/*
	 * Ask other CPUs to stop.  XXX check if there are other CPUs at all?
	 */
	mp_ipi_send_but(mp_whoami(), IPI_STOP);

	kcputs("panic: ");
	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
	kcputs("\n");
	cpu_break();
}

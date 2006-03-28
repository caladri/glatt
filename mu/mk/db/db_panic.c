#include <core/types.h>
#include <core/mp.h>
#include <db/db.h>
#include <io/device/console/console.h>

void
panic(const char *s, ...)
{
	va_list ap;
	int error;

	kcprintf("panic: cpu%u: ", mp_whoami());
	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
	kcputs("\n");

	/*
	 * XXX do in a critical section?
	 */
	if ((PCPU_GET(flags) & PCPU_FLAG_PANICKED) != 0) {
		kcprintf("cpu%u: double panic.\n", mp_whoami());
		for (;;)
			continue;
	}
	PCPU_SET(flags, PCPU_GET(flags) | PCPU_FLAG_PANICKED);

	/*
	 * Ask other CPUs to stop.  XXX check if there are other CPUs at all?
	 */
	mp_ipi_send_but(mp_whoami(), IPI_STOP);

	cpu_break();
}

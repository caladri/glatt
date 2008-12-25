#include <core/types.h>
#include <core/mp.h>
#include <cpu/pcpu.h>
#include <cpu/startup.h>
#include <io/console/console.h>

void
panic(const char *s, ...)
{
	va_list ap;

#ifndef	UNIPROCESSOR
	/*
	 * Ask other CPUs to stop.
	 */
	mp_ipi_send_but(mp_whoami(), IPI_STOP);
#endif

	kcprintf("panic: cpu%u: ", mp_whoami());
	va_start(ap, s);
	kcvprintf(s, ap);
	va_end(ap);
	kcputs("\n");

	if ((PCPU_GET(flags) & PCPU_FLAG_PANICKED) != 0)
		kcprintf("cpu%u: double panic.\n", mp_whoami());
	PCPU_SET(flags, PCPU_GET(flags) | PCPU_FLAG_PANICKED);

	/*
	 * We get to go to the debugger!
	 */
	cpu_break();
}

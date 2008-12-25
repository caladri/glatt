#include <core/types.h>
#include <core/error.h>
#include <core/mp.h>
#include <cpu/startup.h>
#include <db/db.h>
#include <db/db_show.h>
#include <io/console/console.h>

void
db_init(void)
{
	kcprintf("DB: debugger support compiled in.\n");
	db_show_init();
}

void
db_enter(void)
{
	kcprintf("DB: Entering debugger.\n");

#ifndef	UNIPROCESSOR
	/*
	 * Ask other CPUs to stop.
	 */
	mp_ipi_send_but(mp_whoami(), IPI_STOP);
#endif

	db_show();

	kcprintf("DB: Debugger exited, halting...\n");
	cpu_halt();
}

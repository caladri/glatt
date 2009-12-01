#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <core/mp.h>
#include <db/db.h>
#include <db/db_command.h>
#include <io/console/console.h>

void
db_init(void)
{
	kcprintf("DB: debugger support compiled in.\n");
	db_command_init();
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

	critical_enter();
	db_command_enter();
	critical_exit();

	/*
	 * XXX
	 * Resume other CPUs.
	 */
}

int
db_getline(char *buf, size_t len)
{
	unsigned c;
	int error;

	for (c = 0; c < len; c++) {
again:		do {
			error = kcgetc(&buf[c]);
		} while (error == ERROR_AGAIN);

		if (error != 0)
			return (error);

		switch (buf[c]) {
		case '\010':
			if (c != 0) {
				buf[c--] = '\0';
				kcputc('\010');
				kcputc(' ');
				kcputc('\010');
			}
			goto again;
		case '\n':
			buf[c] = '\0';
			kcprintf("\n");
			return (0);
		default:
			kcputc(buf[c]);
			break;
		}
	}
	kcprintf("\nDB: line too long.\n");
	return (ERROR_EXHAUSTED);
}

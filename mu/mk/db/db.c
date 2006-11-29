#include <core/types.h>
#include <core/error.h>
#include <core/mp.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <db/db_action.h>
#include <io/device/console/console.h>

SET(db_actions, struct db_action);

static void db_usage(char);

void
db_enter(void)
{
	kcprintf("DB: Entering debugger.\n");

	/*
	 * Ask other CPUs to stop.  XXX check if there are other CPUs at all?
	 */
	mp_ipi_send_but(mp_whoami(), IPI_STOP);

	/*
	 * XXX acquire a spinlock or halt all other CPUs to prevent lots of
	 * them entering the debugger at the same time.
	 */
	for (;;) {
		struct db_action **actionp;
		int error;
		char ch;

		kcprintf("(db) ");
again:		error = kcgetc(&ch);
		switch (error) {
		case 0:
			break;
		case ERROR_AGAIN:
			goto again;
		default:
			goto bad;
		}
		
		switch (ch) {
		case '\r':
		case '\n':
		case ' ':
			kcprintf("\n");
			continue;
		default:
			kcprintf("%c...\n", ch);
			break;
		}

		for (actionp = SET_BEGIN(db_actions);
		     actionp < SET_END(db_actions); actionp++) {
			if (ch == (*actionp)->dba_char[0]) {
				(*actionp)->dba_function();
				goto again;
			}
		}
		db_usage(ch);
	}
bad:	kcprintf("Read error, can't enter interactive debugger.\n");
	kcprintf("Halting the system, instead.\n");
	cpu_halt();
}

static void
db_usage(char ch)
{
	struct db_action **actionp;

	kcprintf("Available commands:\n");
	for (actionp = SET_BEGIN(db_actions);
	     actionp < SET_END(db_actions); actionp++) {
		/*
		 * Skip hidden commands.
		 */
		if ((*actionp)->dba_help == NULL)
			continue;
		kcprintf("%s\t%s\n",
			 (*actionp)->dba_char, (*actionp)->dba_help);
	}
}

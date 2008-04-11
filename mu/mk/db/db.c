#include <core/types.h>
#include <core/error.h>
#include <core/mp.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <db/db_action.h>
#include <db/db_show.h>
#include <io/console/console.h>

SET(db_actions, struct db_action);

static void db_usage(char);

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
			struct db_action *action;

			action = *actionp;

			if (ch == action->dba_char[0]) {
				while (action->dba_alias != NULL)
					action = action->dba_alias;
				action->dba_function();
				goto next;
			}
		}
		db_usage(ch);
next:		continue;
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
		struct db_action *action;

		action = *actionp;
		if (action->dba_alias == NULL) {
			/* Skip hidden commands.  */
			if (action->dba_help == NULL)
				continue;
			kcprintf("%s %s\n", action->dba_char, action->dba_help);
		} else {
			kcprintf("%s Alias for %s.\n", action->dba_char,
				 action->dba_alias->dba_char);
		}
	}
}

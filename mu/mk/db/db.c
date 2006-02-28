#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <cpu/boot.h>
#include <db/db.h>
#include <db/db_command.h>
#include <io/device/console/console.h>

#define	DB_LINE_MAX	(1024)

SET(db_commands, struct db_command);

static const char *db_gets(void);

static bool db_drunk_debugging;

void
db_enter(void)
{
	struct db_command **commandp, *command;
	const char *line;

	/*
	 * XXX acquire a spinlock or halt all other CPUs to prevent lots of
	 * them entering the debugger at the same time.
	 */
	for (;;) {
again:		if (!db_drunk_debugging)
			kcputs("db> ");
		else
			kcputs("YAAAAAARGH!!!!! ");
		line = db_gets();
		for (commandp = SET_BEGIN(db_commands);
		     commandp < SET_END(db_commands); commandp++) {
			command = *commandp;
			if (strcmp(line, command->dbc_command) == 0) {
				command->dbc_function();
				goto again;
			}
		}
		if (!db_drunk_debugging)
			kcprintf("Unknown command \"%s\"...\n", line);
		else
			kcprintf("You, of all people, should know better.\n");
	}
}

static void
db_command_halt(void)
{
	kcprintf("Halting system from debugger...\n");
	cpu_halt();
}
DB_COMMAND(halt, db_command_halt, "Halt the system.");

static void
db_command_help(void)
{
	struct db_command **commandp, *command;

	kcprintf("Available commands...\n");
	for (commandp = SET_BEGIN(db_commands);
	     commandp < SET_END(db_commands); commandp++) {
		command = *commandp;
		if (!db_drunk_debugging &&
		    (command->dbc_help == NULL || command->dbc_help[0] == '.'))
			continue;
		kcprintf("\t%s\t%s\n", command->dbc_command, command->dbc_help);
	}
}
DB_COMMAND(help, db_command_help, "List available commands.");

static void
db_command_love(void)
{
	if (db_drunk_debugging)
		kcprintf("Make love, not programming errors.\n");
	else
		kcprintf("You only tell me you love me when you're drunk...\n");
	db_drunk_debugging = !db_drunk_debugging;
}
DB_COMMAND(love, db_command_love, ".Is it over when you're sober, is it junk?");

static const char *
db_gets(void)
{
	static char buf[DB_LINE_MAX + 1];
	unsigned o;
	int error;

	buf[0] = '\0';
	for (o = 0; o < DB_LINE_MAX; o++) {
		for (;;) {
			error = kcgetc(&buf[o]);
			if (error == ERROR_AGAIN)
				continue;
			ASSERT(error == 0, "unexpected console error");
			break;
		}
		buf[o + 1] = '\0';
		kcputc(buf[o]); /* XXX too testmips */
		if (buf[o] == '\n') {
			buf[o] = '\0';
			return (buf);
		}
	}
	kcputs("\ndebugger buffer overflowed\n");
	return (buf);
}

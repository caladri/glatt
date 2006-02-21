#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/console/console.h>

#define	DB_LINE_MAX	(1024)

struct db_command {
	const char *dbc_command;
	void (*dbc_function)(void);
	const char *dbc_help;
};

static void db_command_help(void);
static void db_command_love(void);

static const char *db_gets(void);

static struct db_command db_commands[] = {
	{ "help",	db_command_help,
		"List available commands." },
	{ "love",	db_command_love,
		".Is it over when you're sober, is it junk?" },
	{ NULL,		NULL,		NULL }
};

static bool db_drunk_debugging;

void
db_enter(void)
{
	struct db_command *command;
	const char *line;

	for (;;) {
again:		if (!db_drunk_debugging)
			kcputs("db> ");
		else
			kcputs("YAAAAAARGH!!!!! ");
		line = db_gets();
		for (command = db_commands; command->dbc_command != NULL;
		     command++) {
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
db_command_help(void)
{
	struct db_command *command;

	kcprintf("Available commands...\n");
	for (command = db_commands; command->dbc_command != NULL; command++) {
		if (!db_drunk_debugging &&
		    (command->dbc_help == NULL || command->dbc_help[0] == '.'))
			continue;
		kcprintf("\t%s\t%s\n", command->dbc_command, command->dbc_help);
	}
}

static void
db_command_love(void)
{
	if (db_drunk_debugging)
		kcprintf("Make love, not programming errors.\n");
	else
		kcprintf("You only tell me you love me when you're drunk...\n");
	db_drunk_debugging = !db_drunk_debugging;
}

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
		if (buf[o] == '\r') {
			buf[o] = '\0';
			kcputc('\n'); /* XXX too testmips */
			return (buf);
		}
		kcputc(buf[o]); /* XXX too testmips */
	}
	kcputs("\ndebugger buffer overflowed\n");
	return (buf);
}

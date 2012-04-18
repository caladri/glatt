#include <core/types.h>
#include <core/critical.h>
#include <core/error.h>
#include <core/mp.h>
#include <core/string.h>
#include <db/db.h>
#include <db/db_command.h>
#include <core/console.h>

void
db_init(void)
{
	printf("DB: debugger support compiled in.\n");
	db_command_init();
}

void
db_enter(void)
{
	printf("DB: Entering debugger.\n");

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
db_getargs(char *buf, size_t len, unsigned *argcp, const char **argv, size_t avlen, const char *sep)
{
	unsigned argc;
	int error;
	unsigned c;

	error = db_getline(buf, len);
	if (error != 0)
		return (error);

	argc = 0;
	argv[0] = buf;
	c = 0;

	for (;;) {
		/*
		 * Skip leading non-blanks.
		 */
		for (;;) {
			if (buf[c] == '\0' ||
			    strchr(sep, buf[c]) != NULL) {
				break;
			}
			c++;
		}

		/*
		 * Nullify any argument separators.
		 */
		while (buf[c] != '\0' &&
		       strchr(sep, buf[c]) != NULL) {
			buf[c++] = '\0';
		}

		/*
		 * If this argument is not empty, increment
		 * the argument count.
		 */
		if (argv[argc][0] != '\0') {
			if (argc < avlen) {
				argv[++argc] = &buf[c];
			} else {
				/*
				 * Strip any trailing separators.
				 */
				while (buf[c] != '\0' &&
				       strchr(sep, buf[c]) != NULL) {
					buf[c++] = '\0';
				}

				/*
				 * If there are any more arguments,
				 * it's an error.
				 */
				if (buf[c] != '\0') {
					printf("DB: too many arguments.\n");
					return (ERROR_EXHAUSTED);
				}
			}
		} else {
			/*
			 * Just adjust the start of the current argument.
			 */
			argv[argc] = &buf[c];
		}

		/*
		 * If this is the end of the line, return.
		 */
		if (buf[c] == '\0') {
			*argcp = argc;
			return (0);
		}
	}
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
			printf("\n");
			return (0);
		default:
			kcputc(buf[c]);
			break;
		}
	}
	printf("\nDB: line too long.\n");
	return (ERROR_EXHAUSTED);
}

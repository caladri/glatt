#include <core/types.h>
#include <core/console.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>

#include <libmu/common.h>
#include <libmu/ipc_request.h>

int
getchar(void)
{
	struct ipc_request_message req;
	struct ipc_response_message resp;
	int error;

	memset(&req, 0, sizeof req);
	memset(&resp, 0, sizeof resp);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = IPC_PORT_CONSOLE;
	req.msg = CONSOLE_MSG_GETC;
	req.param = 0;
	req.data = NULL;
	req.datalen = 0;

	resp.data = false;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (-1);

	if (resp.error != 0)
		return (-1);

	return (resp.param);
}

void
putchar(int ch)
{
	struct ipc_request_message req;
	int error;

	memset(&req, 0, sizeof req);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = IPC_PORT_CONSOLE;
	req.msg = CONSOLE_MSG_PUTC;
	req.param = ch;
	req.data = NULL;
	req.datalen = 0;

	error = ipc_request(&req, NULL);
	if (error != 0)
		fatal("ipc_request in putchar", error);
}

void
putsn(const char *s, size_t n)
{
	struct ipc_request_message req;
	int error;

	memset(&req, 0, sizeof req);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = IPC_PORT_CONSOLE;
	req.msg = CONSOLE_MSG_PUTS;
	req.param = n;
	req.data = s;
	req.datalen = n;

	error = ipc_request(&req, NULL);
	if (error != 0)
		fatal("ipc_request in putchar", error);
}

void
puts(const char *s)
{
	putsn(s, strlen(s));
}

int
getline(char *buf, size_t len)
{
	unsigned i;
	int ch;

	for (i = 0; i < len; i++) {
again:		ch = getchar();
		if (ch == -1)
			return (ERROR_UNEXPECTED);
		if (ch == '\010') {
			if (i != 0) {
				putsn("\010 \010", 3);
				i--;
			}
			goto again;
		}
		putchar(ch); /* Echo back.  */
		if (ch == '\n') {
			buf[i] = '\0';
			return (0);
		}
		buf[i] = ch;
	}

	return (ERROR_FULL);
}

/*
 * XXX
 * From DB.  Should we really duplicate?
 */
int
getargs(char *buf, size_t len, unsigned *argcp, const char **argv, size_t avlen, const char *sep)
{
	int error;

	error = getline(buf, len);
	if (error != 0)
		return (error);

	error = splitargs(buf, argcp, argv, avlen, sep);
	if (error != 0)
		return (error);

	return (0);
}

int
splitargs(char *buf, unsigned *argcp, const char **argv, size_t avlen, const char *sep)
{
	unsigned argc;
	unsigned c;

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
#if 0
					printf("DB: too many arguments.\n");
#endif
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


void
printf(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
}

void
vprintf(const char *fmt, va_list ap)
{
	char linebuf[1024];

	vsnprintf(linebuf, sizeof linebuf, fmt, ap);
	putsn(linebuf, strlen(linebuf));
}

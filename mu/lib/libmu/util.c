#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

static int debug = 0;

static void printf_putc(void *, char);
static void printf_puts(void *, const char *, size_t);

void
fatal(const char *msg, int error)
{
	printf("fatal: %s: %m\n", msg, error);
	exit();
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
				putchar(ch);
				putchar(' ');
				putchar(ch);
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
	kfvprintf(printf_putc, printf_puts, NULL, fmt, ap);
}

void
hexdump(const void *p, size_t len)
{
	const uint8_t *buf;
	unsigned i;

	buf = p;

	while (len != 0) {
		printf("  ");
		for (i = 0; i < 16; i++) {
			if (len <= i) {
				if (i % 4 == 0)
					printf(" ");
				printf("   ");
				continue;
			}
			if (i != 0) {
				printf(" ");
				if (i % 4 == 0)
					printf(" ");
			}
			if (buf[i] < 0x10)
				printf("0");
			printf("%x", buf[i]);
		}

		printf(" | ");
		for (i = 0; i < 16; i++) {
			if (i != 0 && i % 4 == 0)
				printf(" ");
			if (len <= i) {
				printf(" ");
				continue;
			}
			if (buf[i] >= 0x20 && buf[i] <= 0x7e)
				printf("%c", buf[i]);
			else
				printf(".");
		}
		printf(" |\n");
		if (len <= i)
			break;
		buf += i;
		len -= i;
	}
}

#define	IPC_PORT_PRINT(prefix, port)					\
	do {								\
		switch ((port)) {					\
		case IPC_PORT_UNKNOWN:					\
			printf("%s: Unknown\n", prefix);		\
			break;						\
		case IPC_PORT_NS:					\
			printf("%s: NS\n", prefix);			\
			break;						\
		default:						\
			printf("%s: 0x%jx\n", prefix, (uintmax_t)(port));\
			break;						\
		}							\
	} while (0)

void
ipc_header_print(const struct ipc_header *ipch)
{
	if (debug) {
		printf("Raw message header:\n");
		hexdump(ipch, sizeof *ipch);
	}

	IPC_PORT_PRINT("Message source", ipch->ipchdr_src);
	IPC_PORT_PRINT("Message destination", ipch->ipchdr_dst);
	switch (ipch->ipchdr_right) {
	case IPC_PORT_RIGHT_NONE:
		printf("No port right operation.\n");
		break;
	case IPC_PORT_RIGHT_SEND:
		printf("Grants send right.\n");
		break;
	case IPC_PORT_RIGHT_RECEIVE:
		printf("Grants receive right.\n");
		break;
	case IPC_PORT_RIGHT_SEND_ONCE:
		printf("Grants reply right.\n");
		break;
	default:
		printf("Grants unrecognized right mask: 0x%jx\n", (uintmax_t)ipch->ipchdr_right);
		break;
	}
	printf("Message: 0x%jx\n", (intmax_t)ipch->ipchdr_msg);
	printf("Cookie: 0x%jx\n", (uintmax_t)ipch->ipchdr_cookie);
	printf("Record size: %ju\n", (uintmax_t)ipch->ipchdr_recsize);
	printf("Record count: %ju\n", (uintmax_t)ipch->ipchdr_reccnt);
	printf("Param: 0x%jx\n", (uintmax_t)ipch->ipchdr_param);
}

void
ipc_message_print(const struct ipc_header *ipch, const void *page)
{
	const uint8_t *bytes;
	unsigned i;

	ipc_header_print(ipch);

	printf("Data page: %p\n", page);

	if (ipch->ipchdr_recsize == 0 || ipch->ipchdr_reccnt == 0)
		return;

	if (ipch->ipchdr_recsize > PAGE_SIZE)
		fatal("record size exceeds page size", ERROR_UNEXPECTED);

	if (ipch->ipchdr_reccnt > PAGE_SIZE)
		fatal("record count exceeds page size", ERROR_UNEXPECTED);

	if (ipch->ipchdr_reccnt * ipch->ipchdr_recsize > PAGE_SIZE)
		fatal("record size times record count exceeds page size", ERROR_UNEXPECTED);

	bytes = page;
	for (i = 0; i < ipch->ipchdr_reccnt; i++) {
		printf("Record %u:\n", i);
		hexdump(bytes, ipch->ipchdr_recsize);

		bytes += ipch->ipchdr_recsize;
	}
}

void
ipc_message_drop(const struct ipc_header *ipch, void *page)
{
	int error;

	if (debug) {
		printf("Dropping unexpected message:\n");
		ipc_message_print(ipch, page);
	}
	if (page != NULL) {
		error = vm_page_free(page);
		if (error != 0)
			fatal("vm_page_free failed", error);
	}
}

void *
malloc(size_t len)
{
	void *page;
	int error;

	if (len > PAGE_SIZE)
		fatal("request to allocate more than a page", ERROR_NOT_IMPLEMENTED);
	if (len == 0)
		fatal("request to allocate 0 bytes", ERROR_INVALID);

	error = vm_page_get(&page);
	if (error != 0)
		return (NULL);

	return (page);
}

void
free(void *ptr)
{
	void *page;
	int error;

	if (ptr == NULL)
		fatal("request to free NULL; what do you think this is, plain old C?", ERROR_NOT_IMPLEMENTED);

	page = ptr;

	error = vm_page_free(page);
	if (error != 0)
		fatal("page free failed", error);
}

static void
printf_putc(void *arg, char ch)
{
	(void)arg;
	putchar(ch);
}

static void
printf_puts(void *arg, const char *s, size_t len)
{
	(void)arg;

	putsn(s, len);
}
#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

static int debug = 0;

static void ns_message_print(const struct ipc_header *, const void *);

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
		if (debug) {
			printf("Raw record %u:\n", i);
			hexdump(bytes, ipch->ipchdr_recsize);
		}

		if (ipch->ipchdr_src == IPC_PORT_NS || ipch->ipchdr_dst == IPC_PORT_NS) {
			printf("NS Record %u:\n", i);
			ns_message_print(ipch, bytes);
		} else {
			if (!debug) { 
				printf("Unknown record %u:\n", i);
				hexdump(bytes, ipch->ipchdr_recsize);
			}
		}

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
ns_message_print(const struct ipc_header *ipch, const void *rec)
{
	const struct ns_lookup_request *nslreq;
	const struct ns_lookup_response *nslresp;
	const struct ns_register_request *nsrreq;
	const struct ipc_error_record *err;

#define	NS_GET_REC(val, ipch, rec, typestr)				\
	do {								\
		if ((ipch)->ipchdr_recsize != sizeof *(val)) {		\
			printf("  NS %s record has wrong size.\n", (typestr));\
			return;						\
		}							\
		printf("  NS %s.\n", (typestr));			\
		(val) = (rec);						\
	} while (0)

#define	NS_GET_EMPTY_REC(ipch, typestr)					\
	do {								\
		if ((ipch)->ipchdr_recsize != 0) {			\
			printf("  NS %s record has wrong size.\n", (typestr));\
			return;						\
		}							\
		printf("  NS %s.\n", (typestr));			\
	} while (0)

#define	IPC_ERROR_RECORD(val)						\
	do {								\
		if ((val)->error != 0)					\
			printf("  Error: %m\n", (val)->error);		\
		else							\
			printf("  No error.\n");			\
	} while (0)

#define	NS_PRINT_NAME(val)						\
	do {								\
		if (debug) {						\
			printf("  Raw name:\n");			\
			hexdump((val)->service_name, sizeof (val)->service_name);\
		}							\
		printf("  Name: %s\n", (val)->service_name);		\
	} while (0)

#define	NS_PRINT_PORT(val)	IPC_PORT_PRINT("  Port", (val)->port)

	switch (ipch->ipchdr_msg) {
	case NS_MESSAGE_LOOKUP:
		NS_GET_REC(nslreq, ipch, rec, "lookup request");
		NS_PRINT_NAME(nslreq);
		break;
	case IPC_MSG_REPLY(NS_MESSAGE_LOOKUP):
		NS_GET_REC(nslresp, ipch, rec, "lookup response");
		NS_PRINT_PORT(nslresp);
		break;
	case IPC_MSG_ERROR(NS_MESSAGE_LOOKUP):
		NS_GET_REC(err, ipch, rec, "lookup error");
		IPC_ERROR_RECORD(err);
		break;
	case NS_MESSAGE_REGISTER:
		NS_GET_REC(nsrreq, ipch, rec, "register request");
		NS_PRINT_NAME(nsrreq);
		NS_PRINT_PORT(nsrreq);
		break;
	case IPC_MSG_REPLY(NS_MESSAGE_REGISTER):
		NS_GET_EMPTY_REC(ipch, "lookup response");
		break;
	case IPC_MSG_ERROR(NS_MESSAGE_REGISTER):
		NS_GET_REC(err, ipch, rec, "register error");
		IPC_ERROR_RECORD(err);
		break;
	default:
		printf("  Unhandled NS record:\n");
		hexdump(rec, ipch->ipchdr_recsize);
		break;
	}
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

#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

static int process_rc_line(ipc_port_t, char *);

void
bootstrap_main(void)
{
	ipc_port_t fs, file;
	off_t offset;
	void *page;
	int error;

	printf("The system is bootstrapping.\n");

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	error = open(fs, "/etc/rc", &file);
	if (error != 0)
		fatal("open /etc/rc failed", error);

	offset = 0;
	for (;;) {
		unsigned i;
		size_t len;
		char *buf;

		len = PAGE_SIZE;
		error = read(file, &page, offset, &len);
		if (error != 0) {
			printf("%s: read failed: %m\n", __func__, error);
			return;
		}
		if (len == 0)
			break;

		buf = page;
		for (i = 0; i < len; i++) {
			if (buf[i] == '\0')
				fatal("/etc/rc contains embedded NUL", ERROR_UNEXPECTED);
			if (buf[i] != '\n')
				continue;
			buf[i] = '\0';
			break;
		}
		if (i == len)
			fatal("line longer than a page", ERROR_UNEXPECTED);

		error = process_rc_line(fs, buf);
		if (error != 0)
			fatal("exec /etc/rc failed", error);

		offset += i + 1;
		vm_page_free(page);
	}

	error = close(file);
	if (error != 0)
		fatal("close failed", error);

	printf("Bootstrap exiting.\n");
}

static int
process_rc_line(ipc_port_t fs, char *line)
{
	const char *argv[16];
	ipc_port_t file;
	unsigned argc;
	int error;

	if (line[0] == '\0' || line[0] == '#')
		return (0);

	error = splitargs(line, &argc, argv, 16, " ");
	if (error != 0)
		fatal("splitargs failed", error);

	error = open(fs, argv[0], &file);
	if (error != 0) {
		printf("%s: unable to open %s: %m\n", __func__, argv[0], error);
		return (error);
	}

	/*
	 * XXX wait?
	 */
	error = exec(file, NULL, argc, argv);
	if (error != 0) {
		printf("%s: exec of %s failed: %m\n", __func__, argv[0], error);
		return (error);
	}
	
	error = close(file);
	if (error != 0)
		fatal("close failed", error);

	return (0);
}


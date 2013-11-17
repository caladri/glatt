#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/process.h>

static const char *paths[] = {
	"/bin", "/sbin", NULL
};

static void process_console_line(ipc_port_t);
static void process_file(ipc_port_t, const char *);
static int process_line(ipc_port_t, char *);

void
main(int argc, char *argv[])
{
	ipc_port_t fs;

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	if (argc == 1) {
		printf("MU Experimental Shell.\n");

		for (;;) {
			printf("# ");

			process_console_line(fs);
		}
	} else {
		while (--argc) {
			const char *path = *++argv;

			process_file(fs, path);
		}
	}
}

static void
process_console_line(ipc_port_t fs)
{
	char buf[1024];
	int error;

	error = getline(buf, sizeof buf);
	if (error != 0)
		fatal("getline failed", error);

	error = process_line(fs, buf);
	if (error != 0)
		printf("sh: exec line failed: %m\n", error);
}

static void
process_file(ipc_port_t fs, const char *path)
{
	ipc_port_t file;
	off_t offset;
	void *page;
	int error;

	error = open(fs, path, &file);
	if (error != 0) {
		printf("%s: open %s failed: %m\n", __func__, path, error);
		return;
	}

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
				fatal("file contains embedded NUL", ERROR_UNEXPECTED);
			if (buf[i] != '\n')
				continue;
			buf[i] = '\0';
			break;
		}
		if (i == len)
			fatal("line longer than a page", ERROR_UNEXPECTED);

		error = process_line(fs, buf);
		if (error != 0)
			fatal("exec line failed", error);

		offset += i + 1;
		vm_page_free(page);
	}

	error = close(file);
	if (error != 0)
		fatal("close failed", error);
}

static int
process_line(ipc_port_t fs, char *line)
{
	const char **prefixp;
	const char *argv[128];
	char pathbuf[1024];
	const char *path;
	ipc_port_t file;
	unsigned argc;
	int error;

	if (line[0] == '\0' || line[0] == '#')
		return (0);

	error = splitargs(line, &argc, argv, 128, " ");
	if (error != 0)
		fatal("splitargs failed", error);

	if (strcmp(argv[0], "exit") == 0)
		exit();

	path = argv[0];
	if (path[0] == '/') {
		error = open(fs, path, &file);
		if (error != 0)
			return (error);
	} else {
		file = IPC_PORT_UNKNOWN;

		for (prefixp = paths; *prefixp != NULL; prefixp++) {
			snprintf(pathbuf, sizeof pathbuf, "%s/%s", *prefixp, path);

			error = open(fs, pathbuf, &file);
			if (error != 0)
				continue;
			argv[0] = pathbuf;
			break;
		}
		if (file == IPC_PORT_UNKNOWN)
			return (ERROR_NOT_FOUND);
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

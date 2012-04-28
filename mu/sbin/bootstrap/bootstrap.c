#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

static void display(ipc_port_t, const char *);

void
bootstrap_main(void)
{
	ipc_port_t fs, file;
	const char *argv[16];
	char buf[1024];
	unsigned argc;
	size_t len;
	int error;
	void *p;

	printf("MU Experimental Shell.\n");

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	display(fs, "/etc/motd");

	for (;;) {
		printf("# ");

		error = getargs(buf, sizeof buf, &argc, argv, 16, " ");
		if (error != 0)
			fatal("getargs failed", error);

		if (argc == 0)
			continue;

		if (argc == 1) {
			printf("usage: [hexdump | exec] [file]\n");
			continue;
		}

		error = open(fs, argv[1], &file);
		if (error != 0) {
			printf("Could not open %s: %m\n", argv[1], error);
			continue;
		}

		if (strcmp(argv[0], "hexdump") == 0) {
			/*
			 * XXX
			 * Dump the whole thing.
			 */
			len = PAGE_SIZE;
			error = read(file, &p, 0, &len);
			if (error != 0) {
				printf("Could not read %s: %m\n", buf, error);
				continue;
			}
			hexdump(p, len);
			vm_page_free(p);
		} else if (strcmp(argv[0], "exec") == 0) {
			error = exec(file, NULL, argc - 1, argv + 1);
			if (error != 0) {
				printf("Could not exec %s: %m\n", buf, error);
				continue;
			}
		} else {
			printf("usage: [hexdump | exec] [file]\n");
		}

		error = close(file);
		if (error != 0)
			fatal("close failed", error);
	}
}

static void
display(ipc_port_t fs, const char *path)
{
	ipc_port_t file;
	off_t offset;
	size_t len;
	void *page;
	int error;

	error = open(fs, path, &file);
	if (error != 0) {
		printf("%s: open failed: %m\n", __func__, error);
		return;
	}

	offset = 0;
	for (;;) {
		len = PAGE_SIZE;
		error = read(file, &page, offset, &len);
		if (error != 0) {
			printf("%s: read failed: %m\n", __func__, error);
			return;
		}
		if (len == 0)
			break;

		putsn(page, len);
		offset += len;
	}

	error = close(file);
	if (error != 0) {
		printf("%s: close failed: %m\n", __func__, error);
		return;
	}
}

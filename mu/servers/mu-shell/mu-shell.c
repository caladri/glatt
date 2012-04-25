#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

void
main(void)
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
			error = exec(file);
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

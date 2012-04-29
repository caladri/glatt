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
bootstrap_main(void)
{
	ipc_port_t fs, file;
	const char *argv[16];
	char buf[1024];
	unsigned argc;
	int error;

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

		error = open(fs, argv[0], &file);
		if (error != 0) {
			printf("Could not open %s: %m\n", argv[0], error);
			continue;
		}

		error = exec(file, NULL, argc, argv);
		if (error != 0) {
			printf("Could not exec %s: %m\n", buf, error);
			continue;
		}

		error = close(file);
		if (error != 0)
			fatal("close failed", error);
	}
}

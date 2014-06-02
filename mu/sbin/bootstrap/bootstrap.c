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
	const char *argv[] = {
		"/bin/sh",
		"/etc/rc"
	};
	ipc_port_t fs, file;
	int error;

	printf("The system is bootstrapping.\n");

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	error = open(fs, argv[0], &file);
	if (error != 0)
		fatal("open failed", error);

	error = exec(file, NULL, false, sizeof argv / sizeof argv[0], argv);
	if (error != 0)
		printf("%s: exec of %s failed: %m\n", __func__, argv[0], error);

	error = close(file);
	if (error != 0)
		fatal("close failed", error);

	printf("Bootstrap exiting.\n");
}

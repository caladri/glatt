#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/queue.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/process.h>

static void usage(void);

void
main(int argc, char *argv[])
{
	struct ipc_header ipch;
	ipc_port_t port, root;
	void *page;
	int error;

	if (argc != 2)
		usage();

	while ((root = ns_lookup(argv[1])) == IPC_PORT_UNKNOWN)
		continue;

	error = ipc_port_allocate(&port, IPC_PORT_FLAG_DEFAULT | IPC_PORT_FLAG_PUBLIC);
	if (error != 0)
		fatal("could not allocate port", error);

	error = ns_register("uxserver", port);
	if (error != 0)
		fatal("could not register uxserver service", error);

	for (;;) {
		error = ipc_port_wait(port);
		if (error != 0)
			fatal("ipc_port_wait failed", error);

		error = ipc_port_receive(port, &ipch, &page);
		if (error != 0)
			fatal("ipc_port_receive failed", error);

		error = vm_page_free(page);
		if (error != 0)
			fatal("vm_page_free failed", error);
	}
}

static void
usage(void)
{
	printf("usage: uxserver root-fs\n");
	exit();
}

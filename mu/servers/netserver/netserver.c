#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/queue.h>
#include <core/string.h>
#include <io/network/interface.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/ipc_dispatch.h>
#include <libmu/ipc_request.h>
#include <libmu/process.h>

#include "if.h"
#include "util.h"

static ipc_dispatch_callback_t netserver_ipc_callback;
static void usage(void);

void
main(int argc, char *argv[])
{
	struct ipc_dispatch *id;
	struct if_context *ifc;
	const char *ifname;
	int error;

	if (argc != 5)
		usage();

	ifc = malloc(sizeof *ifc);

	ifname = argv[1];
	error = parse_ip(argv[2], &ifc->ifc_ip);
	if (error != 0)
		fatal("could not parse ip", error);
	error = parse_ip(argv[3], &ifc->ifc_netmask);
	if (error != 0)
		fatal("could not parse netmask", error);
	error = parse_ip(argv[4], &ifc->ifc_gw);
	if (error != 0)
		fatal("could not parse gw", error);

	if_attach(ifc, ifname);

	id = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT | IPC_PORT_FLAG_PUBLIC);

	error = ns_register("netserver", id->id_port);
	if (error != 0)
		fatal("could not register netserver service", error);

	ipc_dispatch_register_default(id, netserver_ipc_callback, NULL);

	ipc_dispatch(id);

	ipc_dispatch_free(id);
}

static void
netserver_ipc_callback(const struct ipc_dispatch *id,
		       const struct ipc_dispatch_handler *idh,
		       const struct ipc_header *ipch, void *page)
{
	(void)id;
	(void)idh;
	(void)ipch;
	(void)page;
	printf("netserver received message.\n");
}

static void
usage(void)
{
	printf("usage: netserver interface ip netmask gw\n");
	exit();
}

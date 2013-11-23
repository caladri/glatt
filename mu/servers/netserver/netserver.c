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

static void usage(void);

void
main(int argc, char *argv[])
{
	struct if_context ifc;
	const char *ifname;
	int error;

	if (argc != 5)
		usage();

	ifname = argv[1];
	error = parse_ip(argv[2], &ifc.ifc_ip);
	if (error != 0)
		fatal("could not parse ip", error);
	error = parse_ip(argv[3], &ifc.ifc_netmask);
	if (error != 0)
		fatal("could not parse netmask", error);
	error = parse_ip(argv[4], &ifc.ifc_gw);
	if (error != 0)
		fatal("could not parse gw", error);

	if_attach(&ifc, ifname);
}

static void
usage(void)
{
	printf("usage: netserver interface ip netmask gw\n");
	exit();
}

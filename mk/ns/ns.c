#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <ipc/data.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <ns/ns.h>
#include <ns/service_directory.h>

static void ns_handle_gibberish(const struct ipc_header *);
static void ns_handle_register(const struct ipc_header *,
			       const struct ipc_data *);
static int ns_handler(void *, struct ipc_header *, struct ipc_data *);

static void
ns_handle_gibberish(const struct ipc_header *reqh)
{
	panic("%s: childishly refusing to respond to nonsense.", __func__);
}

static void
ns_handle_register(const struct ipc_header *reqh, const struct ipc_data *reqd)
{
	struct ns_register_request *req;
	struct ns_register_response resp;
	struct ipc_header ipch;
	struct ipc_data ipcd;
	int error;

	if (reqd == NULL || reqd->ipcd_type != IPC_DATA_TYPE_SMALL ||
	    reqd->ipcd_addr == NULL || reqd->ipcd_len != sizeof *req ||
	    reqd->ipcd_next != NULL) {
		ns_handle_gibberish(reqh);
		return;
	}

	req = reqd->ipcd_addr;

	resp.error = 0;
	resp.cookie = req->cookie;
	strlcpy(resp.service_name, req->service_name, NS_SERVICE_NAME_LENGTH);
	resp.port = IPC_PORT_UNKNOWN;

	error = service_directory_enter(resp.service_name, req->port);
	switch (error) {
	case 0:
		resp.port = req->port;
		break;
	case ERROR_NOT_FREE:
		resp.error = ERROR_NOT_FREE;
		error = service_directory_lookup(resp.service_name, &resp.port);
		if (error != 0)
			panic("%s: service_directory_lookup failed: %m",
			      __func__, error);
		break;
	default:
		resp.error = ERROR_NOT_FREE;
		break;
	}

	ipch = IPC_HEADER_REPLY(reqh);

	ipcd.ipcd_type = IPC_DATA_TYPE_SMALL;
	ipcd.ipcd_addr = &resp;
	ipcd.ipcd_len = sizeof resp;
	ipcd.ipcd_next = NULL;

	error = ipc_port_send(&ipch, &ipcd);
	if (error != 0)
		panic("%s: ipc_port_send failed: %m", __func__, error);
}

static int
ns_handler(void *arg, struct ipc_header *ipch, struct ipc_data *ipcd)
{
	switch (ipch->ipchdr_msg) {
	case NS_MESSAGE_REGISTER:
		ns_handle_register(ipch, ipcd);
		break;
	default:
		/* Don't respond to nonsense.  */
		return (0);
	}

	return (0);
}

static void
ns_startup(void *arg)
{
	int error;

	service_directory_init();

	error = ipc_service("ns", IPC_PORT_NS, ns_handler, NULL);
	if (error != 0)
		panic("%s: ipc_service failed: %m", __func__, error);
}
STARTUP_ITEM(ns, STARTUP_SERVERS, STARTUP_BEFORE, ns_startup, NULL);

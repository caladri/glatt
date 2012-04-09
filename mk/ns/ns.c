#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/console/console.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#include <ns/ns.h>
#include <ns/service_directory.h>

static int ns_handle_gibberish(const struct ipc_header *);
static int ns_handle_lookup(const struct ipc_header *, void *);
static int ns_handle_register(const struct ipc_header *, void *);
static int ns_handler(void *, struct ipc_header *, void *);

static int
ns_handle_gibberish(const struct ipc_header *reqh)
{
	kcprintf("%s: childishly refusing to respond to nonsense.\n", __func__);
	return (ERROR_INVALID);
}

static int
ns_handle_lookup(const struct ipc_header *reqh, void *p)
{
	const struct ns_lookup_request *req;
	struct ns_lookup_response resp;
	struct ns_lookup_error err;
	struct ipc_header ipch;
	ipc_port_t port;
	int error;

	if (p == NULL || reqh->ipchdr_recsize != sizeof *req || reqh->ipchdr_reccnt != 1)
		return (ns_handle_gibberish(reqh));

	req = p;

	error = service_directory_lookup(req->service_name, &port);
	if (error != 0) {
		err.error = error;

		ipch = IPC_HEADER_ERROR(reqh);
		ipch.ipchdr_recsize = sizeof err;
		ipch.ipchdr_reccnt = 1;

		error = ipc_port_send_data(&ipch, &err, sizeof err);
	} else {
		resp.port = port;

		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_recsize = sizeof resp;
		ipch.ipchdr_reccnt = 1;

		error = ipc_port_send_data(&ipch, &resp, sizeof resp);
	}

	if (error != 0) {
		kcprintf("%s: ipc_port_send failed: %m\n", __func__, error);
		return (error);
	}

	return (0);
}

static int
ns_handle_register(const struct ipc_header *reqh, void *p)
{
	const struct ns_register_request *req;
	struct ns_register_error err;
	struct ipc_header ipch;
	int error;

	if (p == NULL || reqh->ipchdr_recsize != sizeof *req || reqh->ipchdr_reccnt != 1)
		return (ns_handle_gibberish(reqh));

	req = p;

	error = service_directory_enter(req->service_name, req->port);
	if (error != 0) {
		err.error = error;

		ipch = IPC_HEADER_ERROR(reqh);
		ipch.ipchdr_recsize = sizeof err;
		ipch.ipchdr_reccnt = 1;

		error = ipc_port_send_data(&ipch, &err, sizeof err);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_recsize = 0;
		ipch.ipchdr_reccnt = 0;

		error = ipc_port_send_data(&ipch, NULL, 0);
	}

	if (error != 0) {
		kcprintf("%s: ipc_port_send failed: %m\n", __func__, error);
		return (error);
	}

	return (0);
}

static int
ns_handler(void *arg, struct ipc_header *ipch, void *p)
{
	switch (ipch->ipchdr_msg) {
	case NS_MESSAGE_LOOKUP:
		return (ns_handle_lookup(ipch, p));
	case NS_MESSAGE_REGISTER:
		return (ns_handle_register(ipch, p));
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static void
ns_startup(void *arg)
{
	int error;

	service_directory_init();

	error = ipc_service("ns", IPC_PORT_NS, IPC_PORT_FLAG_PUBLIC,
			    ns_handler, NULL);
	if (error != 0)
		panic("%s: ipc_service failed: %m", __func__, error);
}
STARTUP_ITEM(ns, STARTUP_SERVERS, STARTUP_BEFORE, ns_startup, NULL);

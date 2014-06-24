#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/ipc_request.h>

ipc_port_t
ns_lookup(const char *service)
{
	struct ipc_response_message resp;
	struct ipc_request_message req;
	struct ns_lookup_request nsreq;
	int error;

	memset(&resp, 0, sizeof resp);
	memset(&req, 0, sizeof req);
	memset(&nsreq, 0, sizeof nsreq);

	strlcpy(nsreq.service_name, service, NS_SERVICE_NAME_LENGTH);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = IPC_PORT_NS;
	req.msg = NS_MSG_LOOKUP;
	req.data = &nsreq;
	req.datalen = sizeof nsreq;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (IPC_PORT_UNKNOWN);

	if (resp.error != 0)
		return (IPC_PORT_UNKNOWN);

	return (resp.param);
}

int
ns_register(const char *service, ipc_port_t port)
{
	struct ipc_response_message resp;
	struct ipc_request_message req;
	struct ns_register_request nsreq;
	int error;

	memset(&resp, 0, sizeof resp);
	memset(&req, 0, sizeof req);
	memset(&nsreq, 0, sizeof nsreq);

	strlcpy(nsreq.service_name, service, NS_SERVICE_NAME_LENGTH);
	nsreq.port = port;

	req.src = IPC_PORT_UNKNOWN;
	req.dst = IPC_PORT_NS;
	req.msg = NS_MSG_REGISTER;
	req.data = &nsreq;
	req.datalen = sizeof nsreq;

	error = ipc_request(&req, &resp);
	if (error != 0)
		return (IPC_PORT_UNKNOWN);

	if (resp.error != 0)
		return (resp.error);

	return (0);
}

#include <core/types.h>
#include <core/error.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <io/console/console.h>
#include <io/network/interface.h>
#ifdef IPC
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>
#endif

#ifdef IPC
static ipc_service_t network_interface_ipc_handler;
static int network_interface_ipc_handle_transmit(struct network_interface *, const struct ipc_header *, const void *);
#endif

int
network_interface_attach(struct network_interface *netif,
			 enum network_interface_type type,
			 const char *name,
			 network_interface_request_handler_t *handler,
			 network_interface_transmit_t *transmit,
			 void *softc)
{
#ifdef IPC
	int error;
#endif

	strlcpy(netif->ni_name, name, sizeof netif->ni_name);
	netif->ni_softc = softc;
	netif->ni_type = type;
	netif->ni_handler = handler;
	netif->ni_transmit = transmit;

	/* XXX Set up based on type?  */

#ifdef IPC
	/* XXX This is gross.  */
	error = ipc_service(netif->ni_name, IPC_PORT_UNKNOWN, IPC_PORT_FLAG_PUBLIC,
			    network_interface_ipc_handler, netif);
	if (error != 0)
		return (error);
#endif

	return (0);
}

#ifdef IPC
static int
network_interface_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct network_interface *netif = softc;

	switch (ipch->ipchdr_msg) {
	case NETWORK_INTERFACE_MSG_TRANSMIT:
		return (network_interface_ipc_handle_transmit(netif, ipch, p));
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static int
network_interface_ipc_handle_transmit(struct network_interface *netif, const struct ipc_header *reqh, const void *p)
{
        struct ipc_header ipch;
	int error;

	if (reqh->ipchdr_reccnt  != 1 || p == NULL)
		return (ERROR_INVALID);

	if (netif->ni_transmit == NULL)
		return (ERROR_NOT_IMPLEMENTED);

	error = netif->ni_transmit(netif->ni_softc, p, reqh->ipchdr_recsize);

        ipch = IPC_HEADER_REPLY(reqh);
        ipch.ipchdr_recsize = sizeof error;
        ipch.ipchdr_reccnt = 1;

        error = ipc_port_send_data(&ipch, &error, sizeof error);
        if (error != 0) {
                kcprintf("%s: ipc_port_send failed: %m\n", __func__, error);
                return (error);
        }

	return (0);
}
#endif

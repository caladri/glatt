#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/spinlock.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/console.h>
#include <io/network/interface.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/service.h>

static ipc_service_t network_interface_ipc_handler;
static int network_interface_ipc_handle_get_info(struct network_interface *, const struct ipc_header *, const void *);
static int network_interface_ipc_handle_receive(struct network_interface *, const struct ipc_header *, const void *);
static int network_interface_ipc_handle_transmit(struct network_interface *, const struct ipc_header *, const void *);

int
network_interface_attach(struct network_interface *netif,
			 enum network_interface_type type,
			 const char *name,
			 network_interface_request_handler_t *handler,
			 network_interface_transmit_t *transmit,
			 void *softc)
{
	int error;

	strlcpy(netif->ni_name, name, sizeof netif->ni_name);
	netif->ni_softc = softc;
	netif->ni_type = type;
	netif->ni_handler = handler;
	netif->ni_transmit = transmit;

	/* XXX Set up based on type?  */

	memset(&netif->ni_receive_header, 0x00, sizeof netif->ni_receive_header);

	/* XXX This is gross.  */
	error = ipc_service(netif->ni_name, IPC_PORT_UNKNOWN, IPC_PORT_FLAG_PUBLIC | IPC_PORT_FLAG_NEW,
			    network_interface_ipc_handler, netif);
	if (error != 0)
		return (error);

	return (0);
}

void
network_interface_receive(struct network_interface *netif,
			  const void *data, size_t datalen)
{
	struct ipc_header ipch;
	int error;

	if (netif->ni_receive_header.ipchdr_dst == IPC_PORT_UNKNOWN)
		return;

	ipch = netif->ni_receive_header;
	ipch.ipchdr_param = datalen;

	error = ipc_port_send_data(&ipch, data, datalen);
	if (error != 0) {
		printf("%s: ipc_port_send failed: %m\n", __func__, error);
		return;
	}
}

static int
network_interface_ipc_handler(void *softc, struct ipc_header *ipch, void *p)
{
	struct network_interface *netif = softc;

	switch (ipch->ipchdr_msg) {
	case NETWORK_INTERFACE_MSG_TRANSMIT:
		return (network_interface_ipc_handle_transmit(netif, ipch, p));
	case NETWORK_INTERFACE_MSG_RECEIVE:
		return (network_interface_ipc_handle_receive(netif, ipch, p));
	case NETWORK_INTERFACE_MSG_GET_INFO:
		return (network_interface_ipc_handle_get_info(netif, ipch, p));
	default:
		/* Don't respond to nonsense.  */
		return (ERROR_INVALID);
	}
}

static int
network_interface_ipc_handle_get_info(struct network_interface *netif, const struct ipc_header *reqh, const void *p)
{
	struct network_interface_get_info_response_header rhdr;
	struct ipc_header ipch;
	uint8_t *data;
	int error;

	if (p != NULL)
		return (ERROR_INVALID);

	if (netif->ni_handler == NULL)
		return (ERROR_NOT_IMPLEMENTED);

	/*
	 * We must be given a reply right.
	 */
	if (reqh->ipchdr_right != IPC_PORT_RIGHT_SEND_ONCE)
		return (ERROR_NO_RIGHT);

	rhdr.type = netif->ni_type;
	switch (netif->ni_type) {
	case NETWORK_INTERFACE_ETHERNET:
		rhdr.addrlen = 6;
		break;
	default:
		rhdr.addrlen = 0;
		rhdr.error = ERROR_NOT_IMPLEMENTED;

		ipch = IPC_HEADER_REPLY(reqh);
		ipch.ipchdr_param = sizeof rhdr;

		error = ipc_port_send_data(&ipch, &rhdr, ipch.ipchdr_param);
		if (error != 0) {
			printf("%s: ipc_port_send failed: %m\n", __func__, error);
			return (error);
		}
		return (0);
	}

	data = malloc(sizeof rhdr + rhdr.addrlen);
	if (data == NULL)
		return (ERROR_EXHAUSTED);

	memset(data + sizeof rhdr, 0, rhdr.addrlen);
	rhdr.error = netif->ni_handler(netif->ni_softc, NETWORK_INTERFACE_GET_ADDRESS,
				       data + sizeof rhdr, rhdr.addrlen);
	memcpy(data, &rhdr, sizeof rhdr);

	ipch = IPC_HEADER_REPLY(reqh);
	ipch.ipchdr_param = sizeof rhdr + rhdr.addrlen;

	error = ipc_port_send_data(&ipch, data, ipch.ipchdr_param);
	if (error != 0) {
		printf("%s: ipc_port_send failed: %m\n", __func__, error);
		free(data);
		return (error);
	}

	free(data);
	return (0);
}

static int
network_interface_ipc_handle_receive(struct network_interface *netif, const struct ipc_header *reqh, const void *p)
{
	struct ipc_header ipch;
	int error;

	if (p != NULL)
		return (ERROR_INVALID);

	if (reqh->ipchdr_right != IPC_PORT_RIGHT_SEND)
		error = ERROR_NO_RIGHT; /* We must be given a send right.  */
	else if (netif->ni_receive_header.ipchdr_dst != IPC_PORT_UNKNOWN)
		error = ERROR_NOT_FREE; /* We already have a receiver set up.  */
	else {
		netif->ni_receive_header = IPC_HEADER_REPLY(reqh);
		netif->ni_receive_header.ipchdr_msg = NETWORK_INTERFACE_MSG_RECEIVE_PACKET;
		netif->ni_receive_header.ipchdr_param = 0;

		error = 0;
	}

	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
	}

	error = ipc_port_send_data(&ipch, NULL, 0);
	if (error != 0) {
		printf("%s: ipc_port_send failed: %m\n", __func__, error);
		return (error);
	}

	return (0);
}

static int
network_interface_ipc_handle_transmit(struct network_interface *netif, const struct ipc_header *reqh, const void *p)
{
	struct ipc_header ipch;
	int error;

	if (p == NULL)
		return (ERROR_INVALID);

	if (netif->ni_transmit == NULL)
		return (ERROR_NOT_IMPLEMENTED);

	error = netif->ni_transmit(netif->ni_softc, p, reqh->ipchdr_param);

	/*
	 * If we weren't given a reply right, don't send back a status.
	 */
	if (reqh->ipchdr_right != IPC_PORT_RIGHT_SEND_ONCE)
		return (0);

	if (error != 0) {
		ipch = IPC_HEADER_ERROR(reqh, error);
	} else {
		ipch = IPC_HEADER_REPLY(reqh);
	}

	error = ipc_port_send_data(&ipch, NULL, 0);
	if (error != 0) {
		printf("%s: ipc_port_send failed: %m\n", __func__, error);
		return (error);
	}

	return (0);
}

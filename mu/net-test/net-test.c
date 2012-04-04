#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <io/network/interface.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

#define	ETHERNET_ADDRESS_SIZE	6

struct ethernet_header {
	uint8_t eh_dhost[ETHERNET_ADDRESS_SIZE];
	uint8_t eh_shost[ETHERNET_ADDRESS_SIZE];
	uint8_t eh_type[2];
};

struct arp_header {
	uint8_t ah_hrd[2];
	uint8_t ah_pro[2];
	uint8_t ah_hln;
	uint8_t ah_pln;
	uint8_t ah_op[2];
	/* Source hardware address.  */
	/* Source protocol address.  */
	/* Target hardware address.  */
	/* Target protocol address.  */
};

struct if_context {
	struct ipc_dispatch *ifc_dispatch;
	const struct ipc_dispatch_handler *ifc_get_info_handler;
	const struct ipc_dispatch_handler *ifc_receive_handler;
	const struct ipc_dispatch_handler *ifc_transmit_handler;
	ipc_port_t ifc_ifport;
};

static void arp_request(struct if_context *, const uint8_t *, uint32_t, uint32_t);
static void if_get_info_callback(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
static void if_receive_callback(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);
static void if_transmit(struct if_context *, const void *, size_t);
static void if_transmit_callback(const struct ipc_dispatch *, const struct ipc_dispatch_handler *, const struct ipc_header *, void *);

void
main(void)
{
	struct if_context ifc;
	int error;

	puts("Starting net-test.\n");

	/*
	 * Wait for the network interface.
	 */
	while ((ifc.ifc_ifport = ns_lookup("tmether0")) == IPC_PORT_UNKNOWN)
		continue;

	ifc.ifc_dispatch = ipc_dispatch_allocate(IPC_PORT_UNKNOWN, IPC_PORT_FLAG_DEFAULT);

	ifc.ifc_get_info_handler = ipc_dispatch_register(ifc.ifc_dispatch, if_get_info_callback, &ifc);
	error = ipc_dispatch_send(ifc.ifc_dispatch, ifc.ifc_get_info_handler, ifc.ifc_ifport, NETWORK_INTERFACE_MSG_GET_INFO, IPC_PORT_RIGHT_SEND_ONCE, NULL, 0);
	if (error != 0)
		fatal("could not send get info message", error);

	ifc.ifc_receive_handler = ipc_dispatch_register(ifc.ifc_dispatch, if_receive_callback, &ifc);
	error = ipc_dispatch_send(ifc.ifc_dispatch, ifc.ifc_receive_handler, ifc.ifc_ifport, NETWORK_INTERFACE_MSG_RECEIVE, IPC_PORT_RIGHT_SEND, NULL, 0);
	if (error != 0)
		fatal("could not send receive registration message", error);

	ifc.ifc_transmit_handler = ipc_dispatch_register(ifc.ifc_dispatch, if_transmit_callback, &ifc);

	ipc_dispatch(ifc.ifc_dispatch);
}

static void
arp_request(struct if_context *ifc, const uint8_t *ether_src, uint32_t ip_src, uint32_t ip_dst)
{
	struct ethernet_header eh;
	struct arp_header ah;
	size_t pktlen;
	uint8_t *pkt;
	uint8_t *p;

	pktlen = sizeof eh + sizeof ah + (2 * sizeof eh.eh_shost) + (2 * sizeof ip_src);
	pkt = malloc(pktlen);
	if (pkt == NULL)
		fatal("could not allocate memory for packet", ERROR_UNEXPECTED);
	p = pkt;

	memset(eh.eh_dhost, 0xff, sizeof eh.eh_dhost);
	if (ether_src != NULL)
		memcpy(eh.eh_shost, ether_src, sizeof eh.eh_shost);
	else
		memset(eh.eh_shost, 0x00, sizeof eh.eh_shost);
	eh.eh_type[0] = 0x08; /* ARP's Ethernet protocol type is 0x0806.  */
	eh.eh_type[1] = 0x06;

	memcpy(p, &eh, sizeof eh);
	p += sizeof eh;

	ah.ah_hrd[0] = 0; /* Ethernet's hardware protocol number for ARP is 1.  */
	ah.ah_hrd[1] = 1;
	ah.ah_pro[0] = 0x08; /* IP's Ethernet protocol type is 0x0800.  */
	ah.ah_pro[1] = 0x00;
	ah.ah_hln = sizeof eh.eh_shost;
	ah.ah_pln = sizeof ip_src;
	ah.ah_op[0] = 0; /* REQUEST is ARP operation 1.  */
	ah.ah_op[1] = 1;

	memcpy(p, &ah, sizeof ah);
	p += sizeof ah;

	memcpy(p, eh.eh_shost, sizeof eh.eh_shost);
	p += sizeof eh.eh_shost;

	memcpy(p, &ip_src, sizeof ip_src);
	p += sizeof ip_src;

	memcpy(p, eh.eh_dhost, sizeof eh.eh_dhost);
	p += sizeof eh.eh_dhost;

	memcpy(p, &ip_dst, sizeof ip_dst);
	p += sizeof ip_dst;

	if_transmit(ifc, pkt, pktlen);
	free(pkt);
}

static void
if_get_info_callback(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	struct network_interface_get_info_response_header rhdr;
	struct if_context *ifc;
	uint8_t *addr;
	unsigned i;

	ifc = idh->idh_softc;

	(void)id;
	(void)idh;

	if (ipch->ipchdr_msg != IPC_MSG_REPLY(NETWORK_INTERFACE_MSG_GET_INFO) ||
	    ipch->ipchdr_reccnt != 1 || ipch->ipchdr_recsize < sizeof rhdr || page == NULL) {
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	memcpy(&rhdr, page, sizeof rhdr);
	addr = page;
	addr += sizeof rhdr;

	printf("Interface address:");
	for (i = 0; i < rhdr.addrlen; i++)
		printf("%c%x%x", i == 0 ? ' ' : ':', (addr[i] & 0xf0) >> 4, addr[i] & 0x0f);
	printf("\n");

	printf("Sending ARP request.\n");
	arp_request(ifc, addr, 0x0a000001, 0x0a0000fe);
}

static void
if_receive_callback(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	int *errorp;

	(void)id;
	(void)idh;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_REPLY(NETWORK_INTERFACE_MSG_RECEIVE):
		if (ipch->ipchdr_reccnt != 1 || ipch->ipchdr_recsize != sizeof *errorp || page == NULL)
			return;
		errorp = page;
		if (*errorp != 0)
			fatal("failed to register receive handler", *errorp);
		return;
	case NETWORK_INTERFACE_MSG_RECEIVE_PACKET:
		if (ipch->ipchdr_reccnt != 1 || ipch->ipchdr_recsize == 0 || page == NULL)
			return;
		break;
	default:
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	printf("Received packet:\n");
	ipc_message_print(ipch, page);
}

static void
if_transmit(struct if_context *ifc, const void *data, size_t datalen)
{
	int error;

	error = ipc_dispatch_send(ifc->ifc_dispatch, ifc->ifc_transmit_handler, ifc->ifc_ifport, NETWORK_INTERFACE_MSG_TRANSMIT, IPC_PORT_RIGHT_NONE, data, datalen);
	if (error != 0)
		fatal("ipc_dispatch_send failed", error);
}

static void
if_transmit_callback(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, const struct ipc_header *ipch, void *page)
{
	(void)id;
	(void)idh;

	printf("Received unexpected message:\n");
	ipc_message_print(ipch, page);
}

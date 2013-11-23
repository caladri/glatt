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

#include "util.h"

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

struct arp_entry {
	uint32_t ae_ip;
	uint8_t ae_ether[ETHERNET_ADDRESS_SIZE];
	STAILQ_ENTRY(struct arp_entry) ae_link;
};

struct if_context {
	struct ipc_dispatch *ifc_dispatch;
	const struct ipc_dispatch_handler *ifc_get_info_handler;
	const struct ipc_dispatch_handler *ifc_receive_handler;
	char ifc_ifname[128];
	ipc_port_t ifc_ifport;

	uint8_t ifc_addr[ETHERNET_ADDRESS_SIZE];
	uint32_t ifc_ip;
	uint32_t ifc_netmask;
	uint32_t ifc_gw;
	STAILQ_HEAD(, struct arp_entry) ifc_arp;
};

static void arp_input(struct if_context *, const void *, size_t);
static void arp_request(struct if_context *, uint32_t);
static void if_get_info_callback(struct if_context *,
				 ipc_parameter_t, void *);
static void if_receive_callback(const struct ipc_dispatch *,
				const struct ipc_dispatch_handler *,
				const struct ipc_header *, void *);
static void if_input(struct if_context *, const void *, size_t);
static void if_transmit(struct if_context *, const void *, size_t);
static void usage(void);

void
main(int argc, char *argv[])
{
	struct ipc_response_message resp;
	struct ipc_request_message req;
	struct if_context ifc;
	int error;

	if (argc != 5)
		usage();

	strlcpy(ifc.ifc_ifname, argv[1], sizeof ifc.ifc_ifname);
	error = parse_ip(argv[2], &ifc.ifc_ip);
	if (error != 0)
		fatal("could not parse ip", error);
	error = parse_ip(argv[3], &ifc.ifc_netmask);
	if (error != 0)
		fatal("could not parse netmask", error);
	error = parse_ip(argv[4], &ifc.ifc_gw);
	if (error != 0)
		fatal("could not parse gw", error);

	/*
	 * Wait for the network interface.
	 */
	while ((ifc.ifc_ifport = ns_lookup(ifc.ifc_ifname)) == IPC_PORT_UNKNOWN)
		continue;

	STAILQ_INIT(&ifc.ifc_arp);

	printf("%s: ipc-port 0x%x\n", ifc.ifc_ifname, ifc.ifc_ifport);

	req.src = IPC_PORT_UNKNOWN;
	req.dst = ifc.ifc_ifport;
	req.msg = NETWORK_INTERFACE_MSG_GET_INFO;
	req.param = 0;
	req.data = NULL;
	req.datalen = 0;
	req.page = NULL;

	resp.data = true;

	error = ipc_request(&req, &resp);
	if (error != 0)
		fatal("could not get interface info", error);
	if (resp.error != 0)
		fatal("driver gave error in response to get info request", resp.error);
	if_get_info_callback(&ifc, resp.param, resp.page);

	ifc.ifc_dispatch = ipc_dispatch_allocate(IPC_PORT_UNKNOWN,
						 IPC_PORT_FLAG_DEFAULT);

	error = ns_register("netserver", ifc.ifc_dispatch->id_port);
	if (error != 0)
		fatal("could not register netserver service", error);

	ifc.ifc_receive_handler = ipc_dispatch_register(ifc.ifc_dispatch,
							if_receive_callback,
							&ifc);
	error = ipc_dispatch_send(ifc.ifc_dispatch, ifc.ifc_receive_handler,
				  ifc.ifc_ifport,
				  NETWORK_INTERFACE_MSG_RECEIVE,
				  IPC_PORT_RIGHT_SEND, NULL, 0);
	if (error != 0)
		fatal("could not send receive registration message", error);

	ipc_dispatch(ifc.ifc_dispatch);

	ipc_dispatch_free(ifc.ifc_dispatch);
}

static void
arp_input(struct if_context *ifc, const void *data, size_t datalen)
{
	uint8_t mac[ETHERNET_ADDRESS_SIZE];
	struct arp_header ah;
	struct arp_entry *ae;
	const uint8_t *p;
	char ip[16];
	unsigned i;

	(void)ifc;

	p = data;
	p += sizeof (struct ethernet_header);
	datalen -= sizeof (struct ethernet_header);

	if (datalen < sizeof ah)
		return;
	memcpy(&ah, p, sizeof ah);
	p += sizeof ah;
	datalen -= sizeof ah;

	if (ah.ah_hrd[0] != 0x00 || ah.ah_hrd[1] != 0x01)
		return;
	if (ah.ah_pro[0] != 0x08 || ah.ah_pro[1] != 0x00)
		return;
	if (ah.ah_hln != sizeof mac)
		return;
	if (ah.ah_pln != sizeof ae->ae_ip)
		return;
	if (ah.ah_op[0] != 0x00 || ah.ah_op[1] != 0x02)
		return;

	/* XXX Just ignore the destination stuff here.  */
	if (datalen < sizeof ae->ae_ether + sizeof ae->ae_ip)
		return;

	ae = malloc(sizeof *ae);

	memcpy(ae->ae_ether, p, sizeof ae->ae_ether);
	p += sizeof ae->ae_ether;
	datalen -= sizeof ae->ae_ether;

	memcpy(&ae->ae_ip, p, sizeof ae->ae_ip);
	p += sizeof ae->ae_ip;
	datalen -= sizeof ae->ae_ip;

	STAILQ_INSERT_TAIL(&ifc->ifc_arp, ae, ae_link);
	format_ip(ip, sizeof ip, ae->ae_ip);
	printf("%s is-at", ip);
	for (i = 0; i < sizeof ae->ae_ether; i++)
		printf("%c%x%x", i == 0 ? ' ' : ':', (ae->ae_ether[i] & 0xf0) >> 4,
		       ae->ae_ether[i] & 0x0f);
	printf("\n");
}

static void
arp_request(struct if_context *ifc, uint32_t ip_dst)
{
	struct ethernet_header eh;
	struct arp_header ah;
	size_t pktlen;
	uint8_t *pkt;
	uint8_t *p;

	pktlen = sizeof eh + sizeof ah + (2 * sizeof eh.eh_shost) +
		(2 * sizeof ifc->ifc_ip);
	pkt = malloc(pktlen);
	if (pkt == NULL)
		fatal("could not allocate memory for packet", ERROR_UNEXPECTED);
	p = pkt;

	memset(eh.eh_dhost, 0xff, sizeof eh.eh_dhost);
	memcpy(eh.eh_shost, ifc->ifc_addr, sizeof eh.eh_shost);
	eh.eh_type[0] = 0x08; /* ARP's Ethernet protocol type is 0x0806.  */
	eh.eh_type[1] = 0x06;

	memcpy(p, &eh, sizeof eh);
	p += sizeof eh;

	ah.ah_hrd[0] = 0; /* Ethernet's protocol number for ARP is 1.  */
	ah.ah_hrd[1] = 1;
	ah.ah_pro[0] = 0x08; /* IP's Ethernet protocol type is 0x0800.  */
	ah.ah_pro[1] = 0x00;
	ah.ah_hln = sizeof eh.eh_shost;
	ah.ah_pln = sizeof ifc->ifc_ip;
	ah.ah_op[0] = 0; /* REQUEST is ARP operation 1.  */
	ah.ah_op[1] = 1;

	memcpy(p, &ah, sizeof ah);
	p += sizeof ah;

	memcpy(p, eh.eh_shost, sizeof eh.eh_shost);
	p += sizeof eh.eh_shost;

	memcpy(p, &ifc->ifc_ip, sizeof ifc->ifc_ip);
	p += sizeof ifc->ifc_ip;

	memcpy(p, eh.eh_dhost, sizeof eh.eh_dhost);
	p += sizeof eh.eh_dhost;

	memcpy(p, &ip_dst, sizeof ip_dst);
	p += sizeof ip_dst;

	if_transmit(ifc, pkt, pktlen);
	free(pkt);
}

static void
if_get_info_callback(struct if_context *ifc, ipc_parameter_t param, void *page)
{
	struct network_interface_get_info_response_header rhdr;
	char ip[16], netmask[16], gw[16];
	uint8_t *addr;
	unsigned i;

	if (param < sizeof rhdr || page == NULL)
		return;

	memcpy(&rhdr, page, sizeof rhdr);
	addr = page;
	addr += sizeof rhdr;

	switch (rhdr.type) {
	case NETWORK_INTERFACE_ETHERNET:
		printf("\tether");
		for (i = 0; i < rhdr.addrlen; i++)
			printf("%c%x%x", i == 0 ? ' ' : ':', (addr[i] & 0xf0) >> 4,
			       addr[i] & 0x0f);
		printf("\n");
		break;
	default:
		printf("\tunknown address:\n");
		hexdump(addr, rhdr.addrlen);
		break;
	}

	if (rhdr.addrlen != sizeof ifc->ifc_addr) {
		printf("%s: preposterous interface address length.\n", __func__);
		return;
	}
	memcpy(ifc->ifc_addr, addr, rhdr.addrlen);

	format_ip(ip, sizeof ip, ifc->ifc_ip);
	format_ip(netmask, sizeof netmask, ifc->ifc_netmask);
	format_ip(gw, sizeof gw, ifc->ifc_gw);
	printf("\tinet %s netmask %s gateway %s\n", ip, netmask, gw);

	arp_request(ifc, ifc->ifc_gw);
}

static void
if_receive_callback(const struct ipc_dispatch *id,
		    const struct ipc_dispatch_handler *idh,
		    const struct ipc_header *ipch, void *page)
{
	struct if_context *ifc;

	ifc = idh->idh_softc;

	(void)id;

	switch (ipch->ipchdr_msg) {
	case IPC_MSG_ERROR(NETWORK_INTERFACE_MSG_RECEIVE):
		if (page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		fatal("failed to register receive handler", ipch->ipchdr_param);
	case IPC_MSG_REPLY(NETWORK_INTERFACE_MSG_RECEIVE):
		if (page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		return;
	case NETWORK_INTERFACE_MSG_RECEIVE_PACKET:
		if (ipch->ipchdr_param == 0 || page == NULL) {
			ipc_message_drop(ipch, page);
			return;
		}
		break;
	default:
		printf("Received unexpected message:\n");
		ipc_message_print(ipch, page);
		return;
	}

	if_input(ifc, page, ipch->ipchdr_param);
}


static void
if_input(struct if_context *ifc, const void *data, size_t datalen)
{
	const struct ethernet_header *eh;

	if (datalen < sizeof *eh) {
		printf("%s: runt packet.\n", __func__);
		return;
	}
	eh = data;

	switch (((uint16_t)eh->eh_type[0] << 8) | eh->eh_type[1]) {
	case 0x0806:
		arp_input(ifc, data, datalen);
		break;
	default:
		printf("%s: unrecognized packet type.\n", __func__);
		return;
	}
}

static void
if_transmit(struct if_context *ifc, const void *data, size_t datalen)
{
	struct ipc_request_message req;
	int error;

	req.src = IPC_PORT_UNKNOWN;
	req.dst = ifc->ifc_ifport;
	req.msg = NETWORK_INTERFACE_MSG_TRANSMIT;
	req.param = datalen;
	req.data = data;
	req.datalen = datalen;

	error = ipc_request(&req, NULL);
	if (error != 0)
		fatal("ipc_request failed", error);
}

static void
usage(void)
{
	printf("usage: netserver interface ip netmask gw\n");
	exit();
}

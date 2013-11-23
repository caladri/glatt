#ifndef	IF_H
#define	IF_H

#define	ETHERNET_ADDRESS_SIZE	6

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

void if_attach(struct if_context *, const char *);
void if_input(struct if_context *, const void *, size_t);
void if_transmit(struct if_context *, const void *, size_t);

#endif /* !IF_H */

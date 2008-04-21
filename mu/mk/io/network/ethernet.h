#ifndef	_IO_NETWORK_ETHERNET_H_
#define	_IO_NETWORK_ETHERNET_H_

#define	ETHERNET_MAC_LENGTH	(48 / 8)

struct ethernet_address {
	uint8_t ea_mac[ETHERNET_MAC_LENGTH];
};

#define	ETHERNET_ADDRESS_MULTICAST(ea)					\
	(((ea)->ea_mac[0] & 1) == 1)

#define	ETHERNET_ADDRESS_BROADCAST(ea)					\
	((ea)->ea_mac[0] == 0xff && (ea)->ea_mac[1] == 0xff &&		\
	 (ea)->ea_mac[2] == 0xff && (ea)->ea_mac[3] == 0xff &&		\
	 (ea)->ea_mac[4] == 0xff && (ea)->ea_mac[5] == 0xff)

#endif /* !_IO_NETWORK_ETHERNET_H_ */

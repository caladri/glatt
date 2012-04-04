#ifndef	_IO_NETWORK_INTERFACE_H_
#define	_IO_NETWORK_INTERFACE_H_

#if defined(MK)
#include <core/btree.h>
#endif

#define	NETWORK_INTERFACE_NAME_LENGTH	(128)

enum network_interface_type {
	NETWORK_INTERFACE_ETHERNET,
};

enum network_interface_request {
	NETWORK_INTERFACE_GET_ADDRESS,
};

/* IPC.  */
/*
 * XXX
 * For now interfaces have their names exported as services, rather than
 * having another layer of enumeration and lookup here.  This is not as
 * abstract as anyone would like.
 */
#define	NETWORK_INTERFACE_MSG_TRANSMIT	(0x00000001)

#if defined(MK)
typedef	int network_interface_request_handler_t(void *,
						enum network_interface_request,
						void *, size_t);

typedef int network_interface_transmit_t(void *, const void *, size_t);

struct network_interface {
	char ni_name[NETWORK_INTERFACE_NAME_LENGTH];
	void *ni_softc;
	enum network_interface_type ni_type;
	network_interface_request_handler_t *ni_handler;
	network_interface_transmit_t *ni_transmit;
};

int network_interface_attach(struct network_interface *,
			     enum network_interface_type,
			     const char *,
			     network_interface_request_handler_t *,
			     network_interface_transmit_t *, void *);
void network_interface_receive(struct network_interface *, const void *, size_t);
#endif

#endif /* !_IO_NETWORK_INTERFACE_H_ */

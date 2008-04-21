#ifndef	_IO_NETWORK_INTERFACE_H_
#define	_IO_NETWORK_INTERFACE_H_

enum network_interface_type {
	NETWORK_INTERFACE_ETHERNET,
};

enum network_interface_request {
	NETWORK_INTERFACE_GET_ADDRESS,
};

typedef	int network_interface_request_handler_t(void *,
						enum network_interface_request,
						void *, size_t);

struct network_interface {
	void *ni_softc;
	enum network_interface_type ni_type;
	network_interface_request_handler_t *ni_handler;
};

int network_interface_attach(struct network_interface *,
			     enum network_interface_type,
			     network_interface_request_handler_t *, void *);


#endif /* !_IO_NETWORK_INTERFACE_H_ */

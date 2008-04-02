#ifndef	_IO_NETWORK_INTERFACE_H_
#define	_IO_NETWORK_INTERFACE_H_

enum network_interface_type {
	NETWORK_INTERFACE_ETHERNET,
};

struct network_interface {
	void *ni_softc;
	enum network_interface_type ni_type;
};

int network_interface_attach(struct network_interface *,
			     enum network_interface_type, void *);


#endif /* !_IO_NETWORK_INTERFACE_H_ */

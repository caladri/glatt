#ifndef	_IO_NETWORK_LAYER_H_
#define	_IO_NETWORK_LAYER_H_

enum network_layer_type {
	NETWORK_LAYER_LINK	= 2,
	NETWORK_LAYER_TRANSPORT	= 3,
};

struct network_layer {
	void *nl_softc;
	enum network_layer_type nl_type;
};

int network_layer_attach(struct network_layer *,
			 enum network_layer_type, void *);

#endif /* !_IO_NETWORK_LAYER_H_ */

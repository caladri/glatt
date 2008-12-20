#include <core/types.h>
#include <io/network/interface.h>

int
network_interface_attach(struct network_interface *netif,
			 enum network_interface_type type,
			 network_interface_request_handler_t *handler,
			 void *softc)
{
	netif->ni_softc = softc;
	netif->ni_type = type;
	netif->ni_handler = handler;

	/* XXX Set up based on type.  */

	return (0);
}

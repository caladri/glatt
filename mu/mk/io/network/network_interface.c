#include <core/types.h>
#include <core/error.h>
#include <io/network/interface.h>

int
network_interface_attach(struct network_interface *netif,
			 enum network_interface_type type, void *softc)
{
	return (ERROR_NOT_IMPLEMENTED);
}

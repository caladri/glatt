#include <core/types.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/system.h>

void
ipc_init(void)
{
	ipc_port_init();
}

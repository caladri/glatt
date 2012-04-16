#include <core/types.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/system.h>
#include <ipc/token.h>

void
ipc_init(void)
{
	ipc_port_init();
	ipc_token_init();
}

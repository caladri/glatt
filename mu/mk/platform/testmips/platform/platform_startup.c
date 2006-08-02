#include <core/types.h>
#include <core/mp.h>
#include <core/startup.h>

void
platform_startup(void)
{
	cpu_startup();
	platform_mp_startup();
	startup_main();
}

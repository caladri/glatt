#include <core/types.h>
#include <core/startup.h>

void
platform_startup(void)
{
	cpu_startup();
	startup_main();
}

void
platform_startup_thread(void)
{
}

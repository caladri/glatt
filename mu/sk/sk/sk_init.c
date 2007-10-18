#include <sk/types.h>
#include <sk/sk.h>

void
sk_init(void)
{
	/*
	 * Early-stage initialization such as memory pool creation.
	 */
	sk_platform_init();
}

void
sk_supervisor_install(void)
{
	sk_platform_supervisor_install();
}

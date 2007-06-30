#include <sk/types.h>
#include <sk/sk.h>
#include <sk/supervisor.h>

void
sk_init(void)
{
	/*
	 * Early-stage initialization such as memory pool creation.
	 */
	sk_platform_init();

	/*
	 * Launch this as if it were any other CPU.
	 */
	Supervisor(Install);
}

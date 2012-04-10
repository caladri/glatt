#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

void
main(void)
{
	char buf[1024];
	int error;

	printf("MU Experimental Shell.\n");
	for (;;) {
		printf("# ");
		error = getline(buf, sizeof buf);
		if (error != 0)
			fatal("getline failed", error);
	}
}

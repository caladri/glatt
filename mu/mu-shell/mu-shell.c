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
	int ch;

	printf("MU Experimental Shell.\n");
	for (;;) {
		printf("# ");
		ch = getchar();
		if (ch == -1)
			fatal("getchar failed", error);
		printf("%c\nYou entered: %x\n", ch, ch);
	}
}

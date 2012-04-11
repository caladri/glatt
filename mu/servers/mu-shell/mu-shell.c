#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <common/common.h>

void
main(void)
{
	ipc_port_t fs, file;
	char buf[1024];
	size_t len;
	int error;
	void *p;

	printf("MU Experimental Shell.\n");

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	for (;;) {
		printf("# ");
		error = getline(buf, sizeof buf);
		if (error != 0)
			fatal("getline failed", error);

		if (buf[0] == '\0')
			continue;

		error = open(fs, buf, &file);
		if (error != 0) {
			printf("Could not open %s: %m\n", buf, error);
			continue;
		}

#if 0
		error = exec(file);
		if (error != 0) {
			printf("Could not exec %s: %m\n", buf, error);
			continue;
		}
#endif

		len = PAGE_SIZE;
		error = read(file, &p, 0, &len);
		if (error != 0) {
			printf("Could not read %s: %m\n", buf, error);
			continue;
		}
		hexdump(p, len);
		vm_page_free(p);

		/* XXX close */
	}
}

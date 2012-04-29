#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

void
main(int argc, char *argv[])
{
	ipc_port_t fs;
	int error;

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	while (--argc) {
		const char *path = *++argv;
		ipc_port_t file;
		off_t offset;
		size_t len;
		void *page;

		error = open(fs, path, &file);
		if (error != 0) {
			printf("Unable to open %s: %m\n", path, error);
			continue;
		}

		offset = 0;
		for (;;) {
			len = PAGE_SIZE;
			error = read(file, &page, offset, &len);
			if (error != 0) {
				printf("%s: read failed: %m\n", __func__, error);
				break;
			}

			if (len == 0)
				break;
			putsn(page, len);
			offset += len;
			vm_page_free(page);
		}

		error = close(file);
		if (error != 0) {
			printf("%s: close failed: %m\n", __func__, error);
			continue;
		}
	}
}

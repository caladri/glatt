#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <fs/fs.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>

static void list_directory(ipc_port_t, const char *);

void
main(int argc, char *argv[])
{
	ipc_port_t fs;

	while ((fs = ns_lookup("ufs0")) == IPC_PORT_UNKNOWN)
		continue;

	argc--;
	argv++;

	if (argc == 0)
		fatal("no working directory", ERROR_NOT_IMPLEMENTED);

	if (argc == 1) {
		list_directory(fs, argv[0]);
		return;
	}

	while (argc--) {
		const char *path = *argv++;
		printf("//%s/%s:\n", "ufs0", path);
		list_directory(fs, path);
	}
}

static void
list_directory(ipc_port_t fs, const char *path)
{
	struct fs_directory_entry de;
	ipc_port_t directory;
	off_t offset;
	int error;

	if (path[0] != '/')
		fatal("no relative paths", ERROR_NOT_IMPLEMENTED);

	error = opendir(fs, path, &directory);
	if (error != 0) {
		printf("Unable to opendir %s: %m\n", path, error);
		return;
	}

	offset = 0;
	for (;;) {
		error = readdir(directory, &de, &offset);
		if (error != 0) {
			printf("%s: readdir failed: %m\n", __func__, error);
			break;
		}

		if (offset == 0 && de.name[0] == '\0')
			break;
		printf("%s\n", de.name);
	}

	error = closedir(directory);
	if (error != 0)
		fatal("closedir failed", error);
}

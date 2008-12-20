#include <core/types.h>
#include <core/error.h>
#include <io/storage/device.h>
#include <io/storage/ufs/mount.h>

struct ufs_superblock { uint8_t trash[8192]; };

static off_t ufs_superblock_offsets[] = {
	65536, 8192, 0, 262144, -1
};

static int ufs_read_superblock(struct storage_device *,
			       struct ufs_superblock *);

int
ufs_mount(struct storage_device *sdev)
{
	struct ufs_superblock sb;
	int error;

	error = ufs_read_superblock(sdev, &sb);
	if (error != 0)
		return (error);

	return (0);
}

static int
ufs_read_superblock(struct storage_device *sdev, struct ufs_superblock *sb)
{
	unsigned i;
	int error;

	for (i = 0; ufs_superblock_offsets[i] != -1; i++) {
		error = storage_device_read(sdev, sb, sizeof *sb,
					    ufs_superblock_offsets[i]);
		if (error != 0)
			continue;
		/* Check superblock.  */
		break;
	}

	if (ufs_superblock_offsets[i] == -1)
		return (ERROR_NOT_FOUND);

	return (0);
}

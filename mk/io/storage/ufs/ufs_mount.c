#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/storage/device.h>
#include <io/storage/ufs/inode.h>
#include <io/storage/ufs/mount.h>
#include <io/storage/ufs/param.h>
#include <io/storage/ufs/superblock.h>
#include <vm/vm.h>
#include <vm/alloc.h>

struct ufs_mount {
	struct storage_device *um_sdev;

	off_t um_sboff;
	struct ufs_superblock um_sb;

	struct ufs2_inode um_in;

	uint8_t um_block[UFS_MAX_BSIZE];
};

static off_t ufs_superblock_offsets[] = UFS_SUPERBLOCK_OFFSETS;

static int ufs_read_inode(struct ufs_mount *, uint64_t);
static int ufs_read_superblock(struct ufs_mount *);

int
ufs_mount(struct storage_device *sdev)
{
	struct ufs_mount *um;
	vaddr_t umaddr;
	int error, error2;

	error = vm_alloc(&kernel_vm, sizeof *um, &umaddr);
	if (error != 0)
		return (error);

	um = (struct ufs_mount *)umaddr;
	um->um_sdev = sdev;
	um->um_sboff = -1;

	error = ufs_read_superblock(um);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *um, umaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error);
		return (error);
	}

	/* Make sure we can read the root inode.  */
	error = ufs_read_inode(um, UFS_ROOT_INODE);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, sizeof *um, umaddr);
		if (error2 != 0)
			panic("%s: vm_free failed: %m", __func__, error);
		return (error);
	}

	return (0);
}

static int
ufs_read_inode(struct ufs_mount *um, uint64_t inode)
{
	uint64_t fsbn = ufs_inode_block(&um->um_sb, inode);
	uint64_t diskblock = UFS_FSBN2DBA(&um->um_sb, fsbn);
	unsigned dbshift = um->um_sb.sb_fshift - um->um_sb.sb_fsbtodb;
	off_t physical = diskblock << dbshift;
	unsigned i = inode % um->um_sb.sb_inopb;
	int error;

	error = storage_device_read(um->um_sdev, um->um_block,
				    1 << um->um_sb.sb_bshift, physical);
	if (error != 0)
		return (error);

	memcpy(&um->um_in, &um->um_block[i * sizeof um->um_in],
	       sizeof um->um_in);

	return (0);
}

static int
ufs_read_superblock(struct ufs_mount *um)
{
	unsigned i;
	int error;

	for (i = 0; ufs_superblock_offsets[i] != -1; i++) {
		error = storage_device_read(um->um_sdev, um->um_block,
					    UFS_SUPERBLOCK_SIZE,
					    ufs_superblock_offsets[i]);
		if (error != 0)
			continue;

		/* Check superblock.  */
		memcpy(&um->um_sb, um->um_block, sizeof um->um_sb);

		switch (um->um_sb.sb_magic) {
		case UFS_SUPERBLOCK_MAGIC1:
		case UFS_SUPERBLOCK_SWAP1:
			return (ERROR_NOT_IMPLEMENTED);
		case UFS_SUPERBLOCK_MAGIC2:
			break;
		case UFS_SUPERBLOCK_SWAP2:
			/* XXX Do I want to do byte-swapping?  */
			return (ERROR_INVALID);
		default:
			continue;
		}

		if (LOG2(UFS_MAX_BSIZE) < um->um_sb.sb_bshift)
			return (ERROR_NOT_IMPLEMENTED);

		um->um_sboff = ufs_superblock_offsets[i];
		break;
	}

	if (um->um_sboff == -1)
		return (ERROR_NOT_FOUND);

	return (0);
}

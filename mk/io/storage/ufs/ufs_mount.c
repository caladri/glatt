#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/storage/device.h>
#include <io/storage/ufs/dir.h>
#include <io/storage/ufs/inode.h>
#include <io/storage/ufs/mount.h>
#include <io/storage/ufs/param.h>
#include <io/storage/ufs/superblock.h>
#include <vm/vm.h>
#include <vm/alloc.h>

#include <io/console/console.h>

struct ufs_directory_context {
	uint64_t d_address;
	struct ufs_directory_entry d_entry;
	char d_name[256];
};

struct ufs_mount {
	struct storage_device *um_sdev;

	off_t um_sboff;
	struct ufs_superblock um_sb;

	uint32_t um_inode;
	struct ufs2_inode um_in;

	struct ufs_directory_context um_dc;

	uint8_t um_block[UFS_MAX_BSIZE];
};

static off_t ufs_superblock_offsets[] = UFS_SUPERBLOCK_OFFSETS;

static int ufs_read_block(struct ufs_mount *, uint64_t);
static int ufs_read_directory(struct ufs_mount *);
static int ufs_read_inode(struct ufs_mount *, uint32_t);
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

	/* And read the contents of the root inode.  */
	for (;;) {
		error = ufs_read_directory(um);
		if (error != 0) {
			error2 = vm_free(&kernel_vm, sizeof *um, umaddr);
			if (error2 != 0)
				panic("%s: vm_free failed: %m", __func__,
				      error);
			return (error);
		}
		kcprintf("%s\n", um->um_dc.d_name);

		if (um->um_dc.d_address == um->um_in.in_size)
			break;
	}

	return (0);
}

static int
ufs_read_block(struct ufs_mount *um, uint64_t off)
{
	uint64_t fsbn, diskblock;
	unsigned dbshift = um->um_sb.sb_fshift - um->um_sb.sb_fsbtodb;
	off_t physical;
	int error;

	error = ufs_inode_map(&um->um_sb, &um->um_in, off, &fsbn);
	if (error != 0)
		return (error);

	diskblock = UFS_FSBN2DBA(&um->um_sb, fsbn);
	physical = diskblock << dbshift;

	error = storage_device_read(um->um_sdev, um->um_block,
				    UFS_BSIZE(&um->um_sb), physical);
	if (error != 0)
		return (error);

	return (0);
}

static int
ufs_read_directory(struct ufs_mount *um)
{
	unsigned offset = um->um_dc.d_address % UFS_BSIZE(&um->um_sb);
	struct ufs_directory_entry *de = &um->um_dc.d_entry;
	int error;

	if (offset == 0) {
		error = ufs_read_block(um, um->um_dc.d_address);
		if (error != 0)
			return (error);
	}

	memcpy(de, &um->um_block[offset], sizeof *de);
	strlcpy(um->um_dc.d_name, (char *)&um->um_block[offset + sizeof *de],
		sizeof um->um_dc.d_name);
	/* XXX Bounds check; make sure it doesn't span blocks.  */
	um->um_dc.d_address += de->de_entrylen;

	return (0);
}

static int
ufs_read_inode(struct ufs_mount *um, uint32_t inode)
{
	uint64_t fsbn = ufs_inode_block(&um->um_sb, inode);
	uint64_t diskblock = UFS_FSBN2DBA(&um->um_sb, fsbn);
	unsigned dbshift = um->um_sb.sb_fshift - um->um_sb.sb_fsbtodb;
	off_t physical = diskblock << dbshift;
	unsigned i = inode % um->um_sb.sb_inopb;
	int error;

	error = storage_device_read(um->um_sdev, um->um_block,
				    UFS_BSIZE(&um->um_sb), physical);
	if (error != 0)
		return (error);

	um->um_dc.d_address = 0;
	um->um_inode = inode;
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

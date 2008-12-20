#include <core/types.h>
#include <core/error.h>
#include <io/storage/device.h>
#include <io/storage/ufs/mount.h>
#include <vm/vm.h>
#include <vm/alloc.h>

#define	UFS_SUPERBLOCK_SIZE	(8192)

#define	UFS_SUPERBLOCK_MAGIC1	(0x00011954)
#define	UFS_SUPERBLOCK_SWAP1	(0x54190100)
#define	UFS_SUPERBLOCK_MAGIC2	(0x19540119)
#define	UFS_SUPERBLOCK_SWAP2	(0x19015419)

struct ufs_superblock {
	int32_t sb_unused1[12];
	int32_t sb_bsize;
	int32_t sb_unused2[6];
	int32_t sb_fmask;
	int32_t sb_bshift;
	int32_t sb_unused3[8];
	int32_t sb_nindir;
	int32_t sb_inopb;
	int32_t sb_unused4[15];
	int32_t sb_ipg;
	int32_t sb_fpg;
	int32_t sb_unused5[202];
	int64_t sb_sblockloc;
	int32_t sb_unused6[18];
	int64_t sb_fsblocks;
	int32_t sb_unused7[62];
	int64_t sb_bmask64;
	int32_t sb_unused8[7];
	int32_t sb_magic;
};
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bsize) == 0x30);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fmask) == 0x4c);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bshift) == 0x50);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_nindir) == 0x74);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_inopb) == 0x78);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_ipg) == 0xb8);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fpg) == 0xbc);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_sblockloc) == 0x3e8);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fsblocks) == 0x438);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bmask64) == 0x538);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_magic) == 0x55c);
COMPILE_TIME_ASSERT(sizeof (struct ufs_superblock) == 1376);
COMPILE_TIME_ASSERT(sizeof (struct ufs_superblock) <= UFS_SUPERBLOCK_SIZE);

struct ufs_mount {
	struct storage_device *um_sdev;

	off_t um_sboff;
	union ufs_mount_superblock {
		struct ufs_superblock ums_sb;
		uint8_t ums_block[UFS_SUPERBLOCK_SIZE];
	} um_sb;
};
#define	UFS_MOUNT_SUPERBLOCK(um)					\
	(&(um)->um_sb.ums_sb)

static off_t ufs_superblock_offsets[] = {
	65536, 8192, 0, 262144, -1
};

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

	return (0);
}

static int
ufs_read_superblock(struct ufs_mount *um)
{
	unsigned i;
	int error;

	for (i = 0; ufs_superblock_offsets[i] != -1; i++) {
		error = storage_device_read(um->um_sdev, um->um_sb.ums_block,
					    UFS_SUPERBLOCK_SIZE,
					    ufs_superblock_offsets[i]);
		if (error != 0)
			continue;

		/* Check superblock.  */

		switch (UFS_MOUNT_SUPERBLOCK(um)->sb_magic) {
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

		um->um_sboff = ufs_superblock_offsets[i];
		break;
	}

	if (um->um_sboff == -1)
		return (ERROR_NOT_FOUND);

	return (0);
}

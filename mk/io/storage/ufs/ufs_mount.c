#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
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
	int32_t sb_unused4[219];
	int64_t sb_sblockloc;
	int32_t sb_unused5[18];
	int64_t sb_fsblocks;
	int32_t sb_unused6[62];
	int64_t sb_bmask64;
	int32_t sb_unused7[7];
	int32_t sb_magic;
};
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bsize) == 0x30);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fmask) == 0x4c);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bshift) == 0x50);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_nindir) == 0x74);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_inopb) == 0x78);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_sblockloc) == 0x3e8);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fsblocks) == 0x438);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bmask64) == 0x538);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_magic) == 0x55c);
COMPILE_TIME_ASSERT(sizeof (struct ufs_superblock) == 1376);
COMPILE_TIME_ASSERT(sizeof (struct ufs_superblock) <= UFS_SUPERBLOCK_SIZE);

struct ufs_mount {
	struct storage_device *um_sdev;

	off_t um_sboff;
	vaddr_t um_sbaddr;
	/* Making this volatile because reads are done to um_sbaddr.  */
	volatile struct ufs_superblock *um_sb;
};

static struct pool ufs_mount_pool;

static off_t ufs_superblock_offsets[] = {
	65536, 8192, 0, 262144, -1
};

static int ufs_read_superblock(struct ufs_mount *);

int
ufs_mount(struct storage_device *sdev)
{
	struct ufs_mount *um;
	int error, error2;

	um = pool_allocate(&ufs_mount_pool);
	um->um_sdev = sdev;
	um->um_sboff = -1;
	um->um_sbaddr = 0;
	um->um_sb = NULL;

	error = vm_alloc(&kernel_vm, UFS_SUPERBLOCK_SIZE, &um->um_sbaddr);
	if (error != 0) {
		pool_free(um);
		return (error);
	}

	um->um_sb = (volatile struct ufs_superblock *)um->um_sbaddr;

	error = ufs_read_superblock(um);
	if (error != 0) {
		error2 = vm_free(&kernel_vm, UFS_SUPERBLOCK_SIZE, um->um_sbaddr);
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
		error = storage_device_read(um->um_sdev, (void *)um->um_sbaddr,
					    UFS_SUPERBLOCK_SIZE,
					    ufs_superblock_offsets[i]);
		if (error != 0)
			continue;

		/* Check superblock.  */

		switch (um->um_sb->sb_magic) {
		case UFS_SUPERBLOCK_MAGIC1:
		case UFS_SUPERBLOCK_MAGIC2:
			break;
			/* XXX Do I want to do byte-swapping?  */
		case UFS_SUPERBLOCK_SWAP1:
		case UFS_SUPERBLOCK_SWAP2:
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

static void
ufs_mount_startup(void *arg)
{
	int error;

	error = pool_create(&ufs_mount_pool, "UFS Mount",
			    sizeof (struct ufs_mount), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}
STARTUP_ITEM(ufs_mount, STARTUP_POOL, STARTUP_FIRST, ufs_mount_startup, NULL);

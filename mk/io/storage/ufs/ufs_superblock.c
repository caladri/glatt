#include <core/types.h>
#include <core/endian.h>
#include <io/storage/ufs/param.h>
#include <io/storage/ufs/superblock.h>

COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_iboff) == 0x10);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_bshift) == 0x50);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fshift) == 0x54);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fsbtodb) == 0x64);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_ipg) == 0xb8);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fpg) == 0xbc);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_sblockloc) == 0x3e8);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_fsblocks) == 0x438);
COMPILE_TIME_ASSERT(offsetof(struct ufs_superblock, sb_magic) == 0x55c);
COMPILE_TIME_ASSERT(sizeof (struct ufs_superblock) == 1376);
COMPILE_TIME_ASSERT(sizeof (struct ufs_superblock) <= UFS_SUPERBLOCK_SIZE);

COMPILE_TIME_ASSERT(UFS_SUPERBLOCK_SIZE <= UFS_MAX_BSIZE);

#define	ufs_superblock_swap32(x)	*(x) = bswap32((uint32_t)*(x))
#define	ufs_superblock_swap64(x)	*(x) = bswap64((uint64_t)*(x))

void
ufs_superblock_swap(struct ufs_superblock *sb)
{
	ufs_superblock_swap32(&sb->sb_iboff);
	ufs_superblock_swap32(&sb->sb_bshift);
	ufs_superblock_swap32(&sb->sb_fshift);
	ufs_superblock_swap32(&sb->sb_fsbtodb);
	ufs_superblock_swap32(&sb->sb_ipg);
	ufs_superblock_swap32(&sb->sb_fpg);
	ufs_superblock_swap64(&sb->sb_sblockloc);
	ufs_superblock_swap64(&sb->sb_fsblocks);
	ufs_superblock_swap32(&sb->sb_magic);
}

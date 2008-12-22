#include <core/types.h>
#include <core/error.h>
#include <io/storage/ufs/inode.h>
#include <io/storage/ufs/superblock.h>

COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_mode) == 0);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_size) == 0x10);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_direct) == 0x70);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_indirect) == 0xd0);
COMPILE_TIME_ASSERT(sizeof (struct ufs2_inode) == 256);

uint64_t
ufs_inode_block(struct ufs_superblock *sb, uint32_t inode)
{
	uint64_t cg = inode / sb->sb_ipg;
	uint64_t ig = (cg * sb->sb_fpg) + sb->sb_iboff;
	uint64_t i = (inode % sb->sb_ipg) / sb->sb_inopb;

	return (ig + (i << (sb->sb_bshift - sb->sb_fshift)));
}

#include <core/types.h>
#include <core/endian.h>
#include <io/storage/ufs/ufs_inode.h>
#include <io/storage/ufs/ufs_superblock.h>

COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_mode) == 0);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_size) == 0x10);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_direct) == 0x70);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_indirect) == 0xd0);
COMPILE_TIME_ASSERT(sizeof (struct ufs2_inode) == 256);

#define	ufs_inode_swap32(x)		*(x) = bswap32((uint32_t)*(x))
#define	ufs_inode_swap64(x)		*(x) = bswap64((uint64_t)*(x))

uint64_t
ufs_inode_block(struct ufs_superblock *sb, uint32_t inode)
{
	uint64_t cg = inode / sb->sb_ipg;
	uint64_t ig = (cg * sb->sb_fpg) + sb->sb_iboff;
	uint64_t i = (inode % sb->sb_ipg) >> UFS_LOG2INOPB(sb);

	return (ig + (i << (sb->sb_bshift - sb->sb_fshift)));
}

void
ufs_inode_swap(struct ufs2_inode *in)
{
	unsigned i;

	ufs_inode_swap32(&in->in_mode);
	ufs_inode_swap64(&in->in_size);
	for (i = 0; i < UFS_INODE_NDIRECT; i++)
		ufs_inode_swap64(&in->in_direct[i]);
	for (i = 0; i < UFS_INODE_NINDIRECT; i++)
		ufs_inode_swap64(&in->in_indirect[i]);
}

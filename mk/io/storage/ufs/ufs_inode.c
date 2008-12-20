#include <core/types.h>
#include <io/storage/ufs/inode.h>

COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_mode) == 0);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_size) == 0x10);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_direct) == 0x70);
COMPILE_TIME_ASSERT(offsetof(struct ufs2_inode, in_indirect) == 0xd0);
COMPILE_TIME_ASSERT(sizeof (struct ufs2_inode) == 256);

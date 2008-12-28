#include <core/types.h>
#include <core/endian.h>
#include <io/storage/ufs/ufs_directory.h>

COMPILE_TIME_ASSERT(offsetof(struct ufs_directory_entry, de_inode) == 0);
COMPILE_TIME_ASSERT(offsetof(struct ufs_directory_entry, de_entrylen) == 0x4);
COMPILE_TIME_ASSERT(offsetof(struct ufs_directory_entry, de_type) == 0x6);
COMPILE_TIME_ASSERT(offsetof(struct ufs_directory_entry, de_namelen) == 0x7);
COMPILE_TIME_ASSERT(sizeof (struct ufs_directory_entry) == 0x8);

#define	ufs_directory_swap16(x)		*(x) = bswap16((uint16_t)*(x))
#define	ufs_directory_swap32(x)		*(x) = bswap32((uint32_t)*(x))

void
ufs_directory_entry_swap(struct ufs_directory_entry *de)
{
	ufs_directory_swap32(&de->de_inode);
	ufs_directory_swap16(&de->de_entrylen);
}

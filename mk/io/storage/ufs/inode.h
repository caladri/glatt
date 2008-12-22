#ifndef	_IO_STORAGE_UFS_INODE_H_
#define	_IO_STORAGE_UFS_INODE_H_

struct ufs_superblock;

#define	UFS_ROOT_INODE		(2)

#define	UFS_INODE_NDIRECT	(12)
#define	UFS_INODE_NINDIRECT	(3)

struct ufs2_inode {
	uint16_t in_mode;
	int16_t in_unused1[7];
	uint64_t in_size;
	int32_t in_unused2[22];
	int64_t in_direct[UFS_INODE_NDIRECT];
	int64_t in_indirect[UFS_INODE_NINDIRECT];
	int32_t in_unused3[5];
};

uint64_t ufs_inode_block(struct ufs_superblock *, uint32_t);

#endif /* !_IO_STORAGE_UFS_INODE_H_*/

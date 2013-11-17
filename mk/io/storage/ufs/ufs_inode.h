#ifndef	_IO_STORAGE_UFS_UFS_INODE_H_
#define	_IO_STORAGE_UFS_UFS_INODE_H_

struct ufs_superblock;

#define	UFS_ROOT_INODE		(2)

#define	UFS_INODE_NDIRECT	(12)
#define	UFS_INODE_NINDIRECT	(3)

#define	UFS_INODE_MODE_FTYPE_MASK	(0xf000)
#define	UFS_INODE_MODE_FTYPE_DIRECTORY	(0x4000)
#define	UFS_INODE_MODE_FTYPE_REGULAR	(0x8000)

struct ufs2_inode {
	uint16_t in_mode;
	int16_t in_unused1[7];
	uint64_t in_size;
	int32_t in_unused2[22];
	int64_t in_direct[UFS_INODE_NDIRECT];
	int64_t in_indirect[UFS_INODE_NINDIRECT];
	int32_t in_unused3[5];
};

uint64_t ufs_inode_block(struct ufs_superblock *, uint32_t) __non_null(1);
void ufs_inode_swap(struct ufs2_inode *) __non_null(1);

#endif /* !_IO_STORAGE_UFS_UFS_INODE_H_ */

#ifndef	_IO_STORAGE_UFS_SUPERBLOCK_H_
#define	_IO_STORAGE_UFS_SUPERBLOCK_H_

#define	UFS_SUPERBLOCK_SIZE	(8192)

#define	UFS_SUPERBLOCK_MAGIC1	(0x00011954)
#define	UFS_SUPERBLOCK_SWAP1	(0x54190100)
#define	UFS_SUPERBLOCK_MAGIC2	(0x19540119)
#define	UFS_SUPERBLOCK_SWAP2	(0x19015419)

struct ufs_superblock {
	int32_t sb_unused1[4];
	int32_t sb_iboff;
	int32_t sb_unused2[15];
	int32_t sb_bshift;
	int32_t sb_fshift;
	int32_t sb_unused3[3];
	int32_t sb_fsbtodb;
	int32_t sb_unused4[20];
	int32_t sb_ipg;
	int32_t sb_fpg;
	int32_t sb_unused5[202];
	int64_t sb_sblockloc;
	int32_t sb_unused6[18];
	int64_t sb_fsblocks;
	int32_t sb_unused7[71];
	int32_t sb_magic;
};

#define	UFS_BSIZE(sb)							\
	((uint64_t)1 << (sb)->sb_bshift)

#define	UFS_FSBN2DBA(sb, blkno)						\
	((uint64_t)(blkno) << (sb)->sb_fsbtodb)

#define	UFS_INOPB(sb)							\
	(1 << UFS_LOG2INOPB((sb)))

/* XXX 8 is the size of the indirect block entries, the address width.  */
#define	UFS_INDIRECTBLOCKS(sb, level)					\
	((uint64_t)1 << ((sb)->sb_bshift - LOG2(8)) * ((level) + 1))

#define	UFS_LOG2INOPB(sb)						\
	((sb)->sb_bshift - LOG2(sizeof (struct ufs2_inode)))

#define	UFS_SUPERBLOCK_OFFSETS						\
	{ 65536, 8192, 0, 262144, -1 }

void ufs_superblock_swap(struct ufs_superblock *);

#endif /* !_IO_STORAGE_UFS_SUPERBLOCK_H_*/

#ifndef	_IO_STORAGE_UFS_SUPERBLOCK_H_
#define	_IO_STORAGE_UFS_SUPERBLOCK_H_

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

#define	UFS_SUPERBLOCK_OFFSETS						\
	{ 65536, 8192, 0, 262144, -1 }

#endif /* !_IO_STORAGE_UFS_SUPERBLOCK_H_*/

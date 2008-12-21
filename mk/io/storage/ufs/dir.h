#ifndef	_IO_STORAGE_UFS_DIR_H_
#define	_IO_STORAGE_UFS_DIR_H_

struct ufs_directory_entry {
	uint32_t de_inode;
	uint16_t de_entrylen;
	uint8_t de_type;
	uint8_t de_namelen;
};

#endif /* !_IO_STORAGE_UFS_DIR_H_ */

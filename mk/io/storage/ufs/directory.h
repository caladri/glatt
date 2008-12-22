#ifndef	_IO_STORAGE_UFS_DIRECTORY_H_
#define	_IO_STORAGE_UFS_DIRECTORY_H_

struct ufs_directory_entry {
	uint32_t de_inode;
	uint16_t de_entrylen;
	uint8_t de_type;
	uint8_t de_namelen;
};

void ufs_directory_entry_swap(struct ufs_directory_entry *);

#endif /* !_IO_STORAGE_UFS_DIRECTORY_H_ */

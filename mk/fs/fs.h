#ifndef	_FS_FS_H_
#define	_FS_FS_H_

/*
 * Global filesystem parameters.
 */
#define	FS_PATH_MAX		(PAGE_SIZE)
#define	FS_NAME_MAX		(1024)

struct fs_directory_entry {
	char name[FS_NAME_MAX];
};

#endif /* !_FS_FS_H_ */

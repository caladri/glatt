#ifndef	_IO_STORAGE_UFS_UFS_MOUNT_H_
#define	_IO_STORAGE_UFS_UFS_MOUNT_H_

struct storage_device;

int ufs_mount(struct storage_device *) __non_null(1);

#endif /* !_IO_STORAGE_UFS_UFS_MOUNT_H_ */

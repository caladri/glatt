#ifndef	_IO_STORAGE_TARFS_TARFS_MOUNT_H_
#define	_IO_STORAGE_TARFS_TARFS_MOUNT_H_

struct storage_device;

int tarfs_mount(struct storage_device *) __non_null(1);

#endif /* !_IO_STORAGE_TARFS_TARFS_MOUNT_H_ */

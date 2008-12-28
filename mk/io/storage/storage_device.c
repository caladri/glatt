#include <core/types.h>
#include <core/error.h>
#include <io/storage/device.h>
#include <io/storage/ufs/ufs_mount.h>

int
storage_device_attach(struct storage_device *sdev, unsigned bsize,
		      storage_device_read_t *read, void *softc)
{
	int error;

	sdev->sd_softc = softc;
	sdev->sd_read = read;
	sdev->sd_bsize = bsize;

	/*
	 * I feel pretty bad about this, but I need to get to the point of
	 * having access to a filesystem sooner rather than later.
	 */
	error = ufs_mount(sdev);
	if (error != 0)
		return (error);

	return (0);
}

int
storage_device_read(struct storage_device *sdev, void *buf, size_t len,
		    off_t off)
{
	int error;

	/* XXX bsize must be a power of two.  Call it bshift instead?  */
	if ((len & (sdev->sd_bsize - 1)) != 0 ||
	    (off & (sdev->sd_bsize - 1)) != 0)
		return (ERROR_INVALID);

	while (len != 0) {
		error = sdev->sd_read(sdev->sd_softc, buf, off);
		if (error != 0)
			return (error);

		buf = (void *)((uintptr_t)buf + sdev->sd_bsize);
		len -= sdev->sd_bsize;
		off += sdev->sd_bsize;
	}

	return (0);
}

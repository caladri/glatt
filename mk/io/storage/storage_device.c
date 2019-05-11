#include <core/types.h>
#include <core/error.h>
#include <core/console.h>
#include <io/storage/device.h>
#ifdef TARFS
#include <io/storage/tarfs/tarfs_mount.h>
#endif
#ifdef UFS
#include <io/storage/ufs/ufs_mount.h>
#endif

int
storage_device_attach(struct storage_device *sdev, unsigned bsize,
		      storage_device_read_t *read, void *softc)
{
#if defined(TARFS) || defined(UFS)
	bool found;
	int error;
#endif

	sdev->sd_softc = softc;
	sdev->sd_read = read;
	sdev->sd_bsize = bsize;

	/*
	 * I feel pretty bad about this, but I need to get to the point of
	 * having access to a filesystem sooner rather than later.
	 */
	found = false;
#ifdef TARFS
	if (!found) {
		error = tarfs_mount(sdev);
		if (error != 0) {
			printf("%s: could not mount tar filesystem.\n", __func__);
		} else {
			found = true;
		}
	}
#endif
#ifdef UFS
	if (!found) {
		error = ufs_mount(sdev);
		if (error != 0) {
			printf("%s: could not mount UFS filesystem.\n", __func__);
		} else {
			found = true;
		}
	}
#endif
	if (!found)
		printf("%s: no filesystem.\n", __func__);

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

#ifndef	_IO_STORAGE_DEVICE_H_
#define	_IO_STORAGE_DEVICE_H_

typedef int storage_device_read_t(void *, void *, off_t);

struct storage_device {
	void *sd_softc;
	storage_device_read_t *sd_read;
	unsigned sd_bsize;
};

int storage_device_attach(struct storage_device *, unsigned,
			  storage_device_read_t *, void *) __non_null(1, 3);
int storage_device_read(struct storage_device *, void *, size_t, off_t) __non_null(1, 2);

#endif /* !_IO_STORAGE_DEVICE_H_ */

#ifndef	_IO_DEVICE_LEAF_H_
#define	_IO_DEVICE_LEAF_H_

struct leaf_device {
	const char *ld_class;
	void *ld_busdata;
};

#endif /* !_IO_DEVICE_LEAF_H_ */

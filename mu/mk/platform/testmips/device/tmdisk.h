#ifndef	_DEVICE_TMDISK_H_
#define	_DEVICE_TMDISK_H_

#define	TEST_DISK_DEV_BASE	(0x13000000)

#define	TEST_DISK_DEV_BLOCKSIZE	(0x0200)

#define	TEST_DISK_DEV_ID_START	(0x0000)
#define	TEST_DISK_DEV_ID_END	(0x0100)

#define	TEST_DISK_DEV_OFFSET	(0x0000)
#define	TEST_DISK_DEV_DISKID	(0x0010)
#define	TEST_DISK_DEV_START	(0x0020)
#define	TEST_DISK_DEV_STATUS	(0x0030)
#define	TEST_DISK_DEV_BLOCK	(0x4000)

#define	TEST_DISK_DEV_FUNCTION(f)					\
	(volatile uint64_t *)XKPHYS_MAP(XKPHYS_UC, TEST_DISK_DEV_BASE + (f))
#define	TEST_DISK_DEV_READ(f)						\
	(volatile uint64_t)*TEST_DISK_DEV_FUNCTION(f)
#define	TEST_DISK_DEV_WRITE(f, v)					\
	*TEST_DISK_DEV_FUNCTION(f) = (v)

#define	TEST_DISK_DEV_START_READ	(0x00)
#define	TEST_DISK_DEV_START_WRITE	(0x01)

#define	TEST_DISK_DEV_STATUS_FAILURE	(0x00)

struct test_disk_busdata {
	unsigned d_id;
};

#endif /* !_DEVICE_TMDISK_H_ */

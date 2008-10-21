#ifndef	_IO_DEVICE_DEVICE_INTERNAL_H_
#define	_IO_DEVICE_DEVICE_INTERNAL_H_

struct bus_instance;

int device_create(struct device **, struct bus_instance *, const char *);
void device_describe(struct device *);
void device_destroy(struct device *);
struct bus_instance *device_parent(struct device *);
int device_setup(struct device *, void *);

#endif /* !_IO_DEVICE_DEVICE_INTERNAL_H_ */

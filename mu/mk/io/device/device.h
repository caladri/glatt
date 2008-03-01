#ifndef	_IO_DEVICE_DEVICE_H_
#define	_IO_DEVICE_DEVICE_H_

struct device;

struct device_interface {
	int (*device_setup)(struct device *, void *);
};

struct device_attachment {
	const char *da_name;
	const char *da_parent;
	struct device_interface *da_interface;
	STAILQ_ENTRY(struct device_attachment) da_link;
};

#define	DEVICE_INTERFACE(ifname)					\
	static struct device_interface __device_interface_ ## ifname =

#define	DEVICE_ATTACHMENT(dev, parent, ifname)				\
	static struct device_attachment __device_attachment_ ## dev = {	\
		.da_name = #dev,					\
		.da_parent = parent,					\
		.da_interface = &__bus_interface_ ## ifname,		\
	};								\
	SET_ADD(device_attachments, __device_attachment_ ## dev)

int device_enumerate(struct bus_instance *, const char *, void *);

#endif /* !_IO_DEVICE_DEVICE_H_ */

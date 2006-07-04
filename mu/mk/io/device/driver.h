#ifndef	_IO_DEVICE_DRIVER_H_
#define	_IO_DEVICE_DRIVER_H_

struct device;
struct driver_attachment;

typedef	int (driver_probe_t)(struct device *);

/*
 * Device driver interfaces, with inheritance.  Driver attachments will be
 * enumerated separately.
 */
struct driver {
	const char *d_name;
	unsigned d_nextunit;
	const char *d_base;
	driver_probe_t *d_probe;
	struct driver *d_parent;
	struct driver *d_children;
	struct driver_attachment *d_attachments;
	struct driver *d_peer;
};
#define	DRIVER(type, base, probe)					\
	static struct driver driver_struct_ ## type = {			\
		.d_name = #type,					\
		.d_nextunit = 0,					\
		.d_base = base,						\
		.d_probe = probe,					\
		.d_parent = NULL,					\
		.d_children = NULL,					\
		.d_peer = NULL,						\
		.d_attachments = NULL,					\
	};								\
	SET_ADD(drivers, driver_struct_ ## type)

struct driver_attachment {
	struct driver *da_driver;
	const char *da_parent;
	struct driver_attachment *da_next;
};
#define	DRIVER_ATTACHMENT(type, parent)					\
	static struct driver_attachment driver_attachment_ ## type = {	\
		.da_driver = &driver_struct_ ## type,			\
		.da_parent = parent,					\
		.da_next = NULL,					\
	};								\
	SET_ADD(driver_attachments, driver_attachment_ ## type)

struct driver *driver_lookup(const char *);
int driver_probe(struct driver *, struct device *);

#endif /* !_IO_DEVICE_DRIVER_H_ */

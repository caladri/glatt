#ifndef	_IO_DEVICE_DRIVER_H_
#define	_IO_DEVICE_DRIVER_H_

#include <core/queue.h>

struct device;
struct driver_attachment;

typedef	int (driver_probe_t)(struct device *);
typedef	int (driver_attach_t)(struct device *);

/*
 * Device driver interfaces, with inheritance.  Driver attachments will be
 * enumerated separately.
 */
struct driver {
	const char *d_name;
	const char *d_desc;
	unsigned d_nextunit;
	const char *d_base;
	driver_probe_t *d_probe;
	driver_attach_t *d_attach;
	struct driver *d_parent;
	STAILQ_HEAD(, struct driver) d_children;
	STAILQ_HEAD(, struct driver_attachment) d_attachments;
	STAILQ_ENTRY(struct driver) d_link;
};
#define	DRIVER(type, desc, base, probe, attach)				\
	static struct driver driver_struct_ ## type = {			\
		.d_name = #type,					\
		.d_desc = desc,						\
		.d_nextunit = 0,					\
		.d_base = base,						\
		.d_probe = probe,					\
		.d_attach = attach,					\
		.d_parent = NULL,					\
		.d_children = STAILQ_HEAD_INITIALIZER(			\
		    _CONCAT(driver_struct_, type).d_children),		\
		.d_attachments = STAILQ_HEAD_INITIALIZER(		\
		    _CONCAT(driver_struct_, type).d_attachments),	\
	};								\
	SET_ADD(drivers, driver_struct_ ## type)

struct driver_attachment {
	struct driver *da_driver;
	const char *da_parent;
	STAILQ_ENTRY(struct driver_attachment) da_link;
};
#define	DRIVER_ATTACHMENT(type, parent)					\
	static struct driver_attachment driver_attachment_ ## type = {	\
		.da_driver = &driver_struct_ ## type,			\
		.da_parent = parent,					\
	};								\
	SET_ADD(driver_attachments, driver_attachment_ ## type)

struct driver *driver_lookup(const char *);
int driver_probe(struct driver *, struct device *);

#endif /* !_IO_DEVICE_DRIVER_H_ */

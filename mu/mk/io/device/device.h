#ifndef	_IO_DEVICE_DEVICE_H_
#define	_IO_DEVICE_DEVICE_H_

#include <core/mutex.h>
#include <core/queue.h>

struct driver;

enum device_state {
	DEVICE_DEAD = 0,
	DEVICE_PROBING,
	DEVICE_ATTACHED,
	DEVICE_DYING
};

struct device {
	struct mutex d_mtx;
	int d_unit;
	struct device *d_parent;
	STAILQ_HEAD(, struct device) d_children;
	STAILQ_ENTRY(struct device) d_link;
	struct driver *d_driver;
	enum device_state d_state;
	const char *d_desc;
	void *d_softc;
};

#define	DEVICE_LOCK(d)		mutex_lock(&(d)->d_mtx)
#define	DEVICE_UNLOCK(d)	mutex_unlock(&(d)->d_mtx)

int device_create(struct device **, struct device *, struct driver *);
int device_init(struct device *, struct device *, struct driver *);
void device_printf(struct device *, const char *, ...);
struct device *device_root(void);

#endif /* !_IO_DEVICE_DEVICE_H_ */

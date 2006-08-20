#ifndef	_IO_DEVICE_DEVICE_H_
#define	_IO_DEVICE_DEVICE_H_

#include <core/queue.h>
#include <core/spinlock.h>

struct driver;

enum device_state {
	DEVICE_DEAD = 0,
	DEVICE_PROBING,
	DEVICE_ATTACHED,
	DEVICE_DYING
};

struct device {
	struct spinlock d_lock;
	unsigned d_unit;
	struct device *d_parent;
	STAILQ_HEAD(, device) d_children;
	STAILQ_ENTRY(device) d_link;
	struct driver *d_driver;
	enum device_state d_state;
	const char *d_desc;
	void *d_softc;
};

#define	DEVICE_LOCK(d)		spinlock_lock(&(d)->d_lock)
#define	DEVICE_UNLOCK(d)	spinlock_unlock(&(d)->d_lock)

int device_create(struct device **, struct device *, struct driver *);
int device_init(struct device *, struct device *, struct driver *);
void device_printf(struct device *, const char *, ...);
struct device *device_root(void);

#endif /* !_IO_DEVICE_DEVICE_H_ */

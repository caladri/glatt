#ifndef	_IO_DEVICE_DEVICE_H_
#define	_IO_DEVICE_DEVICE_H_

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
	struct device *d_parent;
	struct device *d_peer;
	struct device *d_children;
	struct driver *d_driver;
	enum device_state d_state;
	void *d_softc;
};

#define	DEVICE_LOCK(d)		spinlock_lock(&(d)->d_lock)
#define	DEVICE_UNLOCK(d)	spinlock_unlock(&(d)->d_lock)

int device_create(struct device **, struct device *, struct driver *);
int device_init(struct device *, struct device *, struct driver *);
struct device *device_root(void);

#endif /* !_IO_DEVICE_DEVICE_H_ */
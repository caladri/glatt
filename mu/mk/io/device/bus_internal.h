#ifndef	_IO_DEVICE_BUS_INTERNAL_H_
#define	_IO_DEVICE_BUS_INTERNAL_H_

int bus_lookup(struct bus **, const char *);

int bus_instance_create(struct bus_instance **, struct bus_instance *, struct bus *);
void bus_instance_destroy(struct bus_instance *);
int bus_instance_enumerate_children(struct bus_instance *);
const char *bus_instance_name(struct bus_instance *);
struct bus_instance *bus_instance_parent(struct bus_instance *);
void bus_instance_printf(struct bus_instance *, const char *, ...);
int bus_instance_setup(struct bus_instance *, void *);
void *bus_instance_softc(struct bus_instance *);
void *bus_instance_softc_allocate(struct bus_instance *, size_t);

#endif /* !_IO_DEVICE_BUS_INTERNAL_H_ */

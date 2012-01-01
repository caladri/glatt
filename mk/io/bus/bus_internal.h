#ifndef	_IO_BUS_BUS_INTERNAL_H_
#define	_IO_BUS_BUS_INTERNAL_H_

int bus_lookup(struct bus **, const char *);

int bus_instance_create(struct bus_instance **, struct bus_instance *, struct bus *);
void bus_instance_destroy(struct bus_instance *);
int bus_instance_enumerate_children(struct bus_instance *);
const char *bus_instance_name(struct bus_instance *);
struct bus_instance *bus_instance_parent(struct bus_instance *);
void *bus_instance_parent_data(struct bus_instance *);
void *bus_instance_parent_data_allocate(struct bus_instance *, size_t);
void bus_instance_set_description(struct bus_instance *, const char *, va_list);
void bus_instance_vprintf(struct bus_instance *, const char *, va_list);
int bus_instance_setup(struct bus_instance *);
void *bus_instance_softc(struct bus_instance *);
void *bus_instance_softc_allocate(struct bus_instance *, size_t);

#endif /* !_IO_BUS_BUS_INTERNAL_H_ */

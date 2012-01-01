#ifndef	_IO_BUS_BUS_H_
#define	_IO_BUS_BUS_H_

#include <core/queue.h>

struct bus;
struct bus_instance;

struct bus_interface {
	int (*bus_enumerate_children)(struct bus_instance *);
	int (*bus_setup)(struct bus_instance *);
};

struct bus_attachment {
	struct bus *ba_bus;
	const char *ba_name;
	const char *ba_parent;
	struct bus_interface *ba_interface;
	STAILQ_ENTRY(struct bus_attachment) ba_link;
	STAILQ_ENTRY(struct bus_attachment) ba_peers;
};

#define	BUS_INTERFACE(ifname)						\
	static struct bus_interface __bus_interface_ ## ifname =

#define	BUS_ATTACHMENT(bus, parent, ifname)				\
	static struct bus_attachment __bus_attachment_ ## bus = {	\
		.ba_bus = NULL,						\
		.ba_name = #bus,					\
		.ba_parent = parent,					\
		.ba_interface = &__bus_interface_ ## ifname,		\
	};								\
	SET_ADD(bus_attachments, __bus_attachment_ ## bus)

int bus_enumerate_child(struct bus_instance *, const char *, struct bus_instance **);
int bus_enumerate_child_generic(struct bus_instance *, const char *);
int bus_enumerate_children(struct bus_instance *);
const char *bus_name(struct bus_instance *);
struct bus_instance *bus_parent(struct bus_instance *);
void *bus_parent_data(struct bus_instance *);
void *bus_parent_data_allocate(struct bus_instance *, size_t);
void bus_printf(struct bus_instance *, const char *, ...);
void bus_set_description(struct bus_instance *, const char *, ...);
int bus_setup_child(struct bus_instance *);
void *bus_softc(struct bus_instance *);
void *bus_softc_allocate(struct bus_instance *, size_t);
void bus_vprintf(struct bus_instance *, const char *, va_list);

#endif /* !_IO_BUS_BUS_H_ */

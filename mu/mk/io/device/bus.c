#include <core/types.h>
#include <core/error.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <db/db.h>
#include <io/device/bus.h>
#include <io/device/console/console.h>

struct bus_instance;

struct bus {
	const char *bus_name;
	STAILQ_HEAD(, struct bus_attachment) bus_attachments;
	STAILQ_HEAD(, struct bus_instance) bus_instances;
	STAILQ_HEAD(, struct bus_attachment) bus_children;
};
static struct pool bus_pool;

struct bus_instance {
	struct bus_instance *bi_parent;
	struct bus_attachment *bi_attachment;
	STAILQ_ENTRY(struct bus_instance) bi_link;
};
static struct pool bus_instance_pool;

SET(bus_attachments, struct bus_attachment);

static struct bus *bus_root;

static int bus_create(struct bus **, struct bus_attachment *);
static int bus_find(struct bus **, const char *);
static int bus_link(struct bus_attachment *);

static int bus_instance_create(struct bus_instance **, struct bus_instance *, struct bus_attachment *);

static int
bus_create(struct bus **busp, struct bus_attachment *attachment)
{
	struct bus *bus;
	int error;

	ASSERT(attachment->ba_bus == NULL,
	       "An attachment can belong to only one bus.");

	error = bus_find(&bus, attachment->ba_name);
	if (error != 0) {
		if (error != ERROR_NOT_FOUND)
			return (error);
		bus = pool_allocate(&bus_pool);
		bus->bus_name = attachment->ba_name;
		STAILQ_INIT(&bus->bus_attachments);
		STAILQ_INIT(&bus->bus_instances);
		STAILQ_INIT(&bus->bus_children);
	}
	attachment->ba_bus = bus;
	STAILQ_INSERT_TAIL(&bus->bus_attachments, attachment, ba_link);
	if (busp != NULL)
		*busp = bus;
	return (0);
}

static int
bus_find(struct bus **busp, const char *name)
{
	struct bus_attachment **attachmentp;

	for (attachmentp = SET_BEGIN(bus_attachments);
	     attachmentp < SET_END(bus_attachments); attachmentp++) {
		struct bus_attachment *attachment = *attachmentp;

		if (strcmp(attachment->ba_name, name) != 0)
			continue;
		if (attachment->ba_bus == NULL)
			continue;
		*busp = attachment->ba_bus;
		return (0);
	}
	return (ERROR_NOT_FOUND);
}

static int
bus_link(struct bus_attachment *attachment)
{
	struct bus *bus, *parent;
	int error;

	ASSERT(attachment->ba_bus != NULL,
	       "Attachment must already have a bus.");
	bus = attachment->ba_bus;

	if (attachment->ba_parent == NULL) {
		ASSERT(bus == bus_root,
		       "Only root bus may not have a parent.");
		kcprintf("bus attachment %s/%s will be root bus\n",
			 attachment->ba_parent, attachment->ba_name);
		return (0);
	}

	error = bus_find(&parent, attachment->ba_parent);
	if (error != 0)
		panic("%s: cannot find parent bus.", __func__);
	kcprintf("bus attachment %s/%s will be able to connect to bus %s\n",
		 attachment->ba_parent, attachment->ba_name, parent->bus_name);

	STAILQ_INSERT_TAIL(&parent->bus_children, attachment, ba_peers);
	return (0);
}

static int
bus_instance_create(struct bus_instance **bip, struct bus_instance *parent,
		    struct bus_attachment *attachment)
{
	struct bus_instance *bi;
	int error;

	ASSERT(parent != NULL || attachment->ba_parent == NULL,
	       "Must have a parent unless this attachment doesn't.");

	if (attachment->ba_parent != NULL) {
		struct bus *parentbus;

		error = bus_find(&parentbus, attachment->ba_parent);
		if (error != 0)
			return (error);
		if (parentbus != parent->bi_attachment->ba_bus)
			return (ERROR_INVALID);
	}

	bi = pool_allocate(&bus_instance_pool);
	bi->bi_parent = parent;
	bi->bi_attachment = attachment;
	STAILQ_INSERT_TAIL(&attachment->ba_bus->bus_instances, bi, bi_link);
	return (0);
}

static void
bus_compile(void *arg)
{
	struct bus_attachment **attachmentp;
	int error;

	error = pool_create(&bus_pool, "bus", sizeof (struct bus),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	error = pool_create(&bus_instance_pool, "bus instance",
			    sizeof (struct bus_instance),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	for (attachmentp = SET_BEGIN(bus_attachments);
	     attachmentp < SET_END(bus_attachments); attachmentp++) {
		struct bus_attachment *attachment = *attachmentp;

		if (attachment->ba_parent == NULL) {
			if (bus_root != NULL)
				panic("%s: redundant root bus attachment.",
				      __func__);
			error = bus_create(&bus_root, attachment);
			if (error != 0)
				panic("%s: bus_create failed: %m", __func__,
				      error);
		} else {
			error = bus_create(NULL, attachment);
			if (error != 0)
				panic("%s: bus_create failed: %m", __func__,
				      error);
		}
	}
	if (bus_root == NULL)
		panic("%s: must have a root bus.", __func__);

	for (attachmentp = SET_BEGIN(bus_attachments);
	     attachmentp < SET_END(bus_attachments); attachmentp++) {
		struct bus_attachment *attachment = *attachmentp;

		error = bus_link(attachment);
		if (error != 0)
			panic("%s: bus_link failed: %m", __func__, error);
	}
}
STARTUP_ITEM(bus_compile, STARTUP_DRIVERS, STARTUP_BEFORE, bus_compile, NULL);

static void
bus_enumerate(void *arg)
{
	struct bus_instance *bi;
	int error;

	ASSERT(bus_root != NULL, "Must have a root bus!");

	error = bus_instance_create(&bi, NULL,
				    STAILQ_FIRST(&bus_root->bus_attachments));
	if (error != 0)
		panic("%s: bus_instance_create failed: %m", __func__, error);
}
STARTUP_ITEM(bus_enumerate, STARTUP_DRIVERS, STARTUP_FIRST, bus_enumerate, NULL);

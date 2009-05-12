#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#ifdef DB
#include <db/db_command.h>
#endif
#include <io/device/bus.h>
#include <io/device/bus_internal.h>
#include <io/console/console.h>

#ifdef DB
DB_COMMAND_TREE(bus, root, bus);
#endif

struct bus {
	const char *bus_name;
	STAILQ_HEAD(, struct bus_attachment) bus_attachments;
	STAILQ_HEAD(, struct bus_instance) bus_instances;
	STAILQ_HEAD(, struct bus_attachment) bus_children;
};
static struct pool bus_pool;

struct bus_instance {
	STAILQ_HEAD(, struct bus_instance) bi_children;
	struct bus_instance *bi_parent;
	struct bus_attachment *bi_attachment;
	void *bi_pdata;
	void *bi_softc;
	STAILQ_ENTRY(struct bus_instance) bi_peer;
	STAILQ_ENTRY(struct bus_instance) bi_link;
	char bi_description[128];
};
static struct pool bus_instance_pool;

SET(bus_attachments, struct bus_attachment);

static struct bus *bus_root;

static int bus_create(struct bus **, struct bus_attachment *);
static int bus_link(struct bus_attachment *);

static int bus_attachment_find(struct bus_attachment **, struct bus *, struct bus *);

static void bus_instance_describe(struct bus_instance *);
static void bus_instance_print(struct bus_instance *);
static void bus_instance_printf(struct bus_instance *, const char *, ...);

int
bus_lookup(struct bus **busp, const char *name)
{
	struct bus_attachment **attachmentp;

	SET_FOREACH(attachmentp, bus_attachments) {
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

int
bus_instance_create(struct bus_instance **bip, struct bus_instance *parent,
		    struct bus *bus)
{
	struct bus_attachment *attachment;
	struct bus_instance *bi;
	struct bus *parentbus;
	int error;

	parentbus = parent == NULL ? NULL : parent->bi_attachment->ba_bus;
	error = bus_attachment_find(&attachment, parentbus, bus);
	if (error != 0)
		return (error);

	ASSERT(parent != NULL || attachment->ba_parent == NULL,
	       "Must have a parent unless this attachment doesn't.");

	bi = pool_allocate(&bus_instance_pool);
	STAILQ_INIT(&bi->bi_children);
	bi->bi_parent = parent;
	bi->bi_attachment = attachment;
	bi->bi_pdata = NULL;
	bi->bi_softc = NULL;
	if (parent != NULL)
		STAILQ_INSERT_TAIL(&parent->bi_children, bi, bi_peer);
	STAILQ_INSERT_TAIL(&attachment->ba_bus->bus_instances, bi, bi_link);
	bi->bi_description[0] = '\0';
	if (bip != NULL)
		*bip = bi;
	return (0);
}

/*
 * XXX Make sure there are no children.
 */
void
bus_instance_destroy(struct bus_instance *bi)
{
	struct bus_instance *parent;
	struct bus *bus;

	bus = bi->bi_attachment->ba_bus;
	parent = bi->bi_parent;
	STAILQ_REMOVE(&bus->bus_instances, bi, struct bus_instance, bi_link);
	if (parent != NULL)
		STAILQ_REMOVE(&parent->bi_children, bi, struct bus_instance, bi_peer);
	if (bi->bi_pdata != NULL)
		free(bi->bi_pdata);
	if (bi->bi_softc != NULL)
		free(bi->bi_softc);
	if (!STAILQ_EMPTY(&bi->bi_children))
		NOTREACHED();
	pool_free(bi);
}

int
bus_instance_enumerate_children(struct bus_instance *bi)
{
	struct bus_attachment *attachment;
	struct bus *bus;
	int error;

	bus = bi->bi_attachment->ba_bus;

	STAILQ_FOREACH(attachment, &bus->bus_children, ba_peers) {
		error = bus_enumerate_child_generic(bi, attachment->ba_bus->bus_name);
		if (error != 0)
			bus_instance_printf(bi, "bus_enumerate_child_generic (%s) failed: %m", attachment->ba_bus->bus_name, error);
	}
	return (0);
}

const char *
bus_instance_name(struct bus_instance *bi)
{
	return (bi->bi_attachment->ba_name);
}

struct bus_instance *
bus_instance_parent(struct bus_instance *bi)
{
	return (bi->bi_parent);
}

void *
bus_instance_parent_data(struct bus_instance *bi)
{
	ASSERT(bi->bi_pdata != NULL, "Don't ask for what you haven't created.");
	return (bi->bi_pdata);
}

void *
bus_instance_parent_data_allocate(struct bus_instance *bi, size_t size)
{
	ASSERT(bi->bi_pdata == NULL, "Can't create two parent datas.");
	bi->bi_pdata = malloc(size);
	return (bi->bi_pdata);
}

void
bus_instance_set_description(struct bus_instance *bi, const char *fmt, va_list ap)
{
	vsnprintf(bi->bi_description, sizeof bi->bi_description, fmt, ap);
}

int
bus_instance_setup(struct bus_instance *bi)
{
	int error;

	error = bi->bi_attachment->ba_interface->bus_setup(bi);
	if (error != 0) {
#ifdef VERBOSE
		bus_instance_printf(bi, "bus_setup: %m", error);
#endif
		return (error);
	}
	bus_instance_describe(bi);
	if (bi->bi_attachment->ba_interface->bus_enumerate_children != NULL) {
		error = bi->bi_attachment->ba_interface->bus_enumerate_children(bi);
		if (error != 0) {
			bus_instance_printf(bi, "bus_enumerate_children: %m", error);
		}
	} else {
		error = bus_instance_enumerate_children(bi);
		if (error != 0) {
			bus_instance_printf(bi, "bus_instance_enumerate_children: %m", error);
		}
	}
	return (0);
}

void *
bus_instance_softc(struct bus_instance *bi)
{
	ASSERT(bi->bi_softc != NULL, "Don't ask for what you haven't created.");
	return (bi->bi_softc);
}

void *
bus_instance_softc_allocate(struct bus_instance *bi, size_t size)
{
	ASSERT(bi->bi_softc == NULL, "Can't create two softcs.");
	bi->bi_softc = malloc(size);
	return (bi->bi_softc);
}

void
bus_instance_vprintf(struct bus_instance *bi, const char *fmt, va_list ap)
{
	bus_instance_print(bi);
	kcprintf(": ");
	kcvprintf(fmt, ap);
	kcprintf("\n");
}

static int
bus_create(struct bus **busp, struct bus_attachment *attachment)
{
	struct bus *bus;
	int error;

	ASSERT(attachment->ba_bus == NULL,
	       "An attachment can belong to only one bus.");

	error = bus_lookup(&bus, attachment->ba_name);
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
bus_link(struct bus_attachment *attachment)
{
	struct bus *bus, *parent;
	int error;

	ASSERT(attachment->ba_bus != NULL,
	       "Attachment must already have a bus.");
	bus = attachment->ba_bus;

	if (attachment->ba_parent == NULL) {
		return (0);
	}

	error = bus_lookup(&parent, attachment->ba_parent);
	if (error != 0)
		panic("%s: cannot find parent bus.", __func__);
	STAILQ_INSERT_TAIL(&parent->bus_children, attachment, ba_peers);
	return (0);
}

static int
bus_attachment_find(struct bus_attachment **attachment2p, struct bus *parent,
		    struct bus *child)
{
	struct bus_attachment **attachmentp;

	SET_FOREACH(attachmentp, bus_attachments) {
		struct bus_attachment *attachment = *attachmentp;

		if (attachment->ba_bus == NULL) {
			if (strcmp(attachment->ba_name, child->bus_name) != 0)
				continue;
		} else {
			if (attachment->ba_bus != child)
				continue;
		}

		if (parent != NULL) {
			if (attachment->ba_parent == NULL)
				continue;
			if (strcmp(attachment->ba_parent, parent->bus_name) != 0)
				continue;
		} else {
			if (attachment->ba_parent != NULL)
				continue;
		}

		*attachment2p = attachment;
		return (0);
	}

	if (parent == NULL)
		return (ERROR_NOT_FOUND);

	/*
	 * Now look for a wildcard match.
	 */
	SET_FOREACH(attachmentp, bus_attachments) {
		struct bus_attachment *attachment = *attachmentp;

		if (attachment->ba_bus == NULL) {
			if (strcmp(attachment->ba_name, child->bus_name) != 0)
				continue;
		} else {
			if (attachment->ba_bus != child)
				continue;
		}

		if (attachment->ba_parent != NULL)
			continue;
		*attachment2p = attachment;
		return (0);
	}
	return (ERROR_NOT_FOUND);
}

static void
bus_instance_describe(struct bus_instance *bi)
{
	if (bi->bi_description[0] == '\0') {
#ifdef VERBOSE
		bus_instance_printf(bi, "<%m>", ERROR_NOT_IMPLEMENTED);
#endif
		return;
	}
	bus_instance_printf(bi, "%s", bi->bi_description);
}

static void
bus_instance_print(struct bus_instance *bi)
{
	/* XXX Unit numbers, or similar.  */
	kcprintf("%s", bi->bi_attachment->ba_bus->bus_name);
	if (bi->bi_parent != NULL) {
		kcprintf("@");
		bus_instance_print(bi->bi_parent);
	}
}

static void
bus_instance_printf(struct bus_instance *bi, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bus_instance_vprintf(bi, fmt, ap);
	va_end(ap);
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

	SET_FOREACH(attachmentp, bus_attachments) {
		struct bus_attachment *attachment = *attachmentp;

		if (attachment->ba_parent == NULL &&
		    strcmp(attachment->ba_name, "root") == 0) {
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

	SET_FOREACH(attachmentp, bus_attachments) {
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

	error = bus_instance_create(&bi, NULL, bus_root);
	if (error != 0)
		panic("%s: bus_instance_create failed: %m", __func__, error);

	error = bus_instance_setup(bi);
	if (error != 0)
		panic("%s: bus_instance_setup failed: %m", __func__, error);
}
STARTUP_ITEM(bus_enumerate, STARTUP_DRIVERS, STARTUP_FIRST, bus_enumerate, NULL);

#ifdef DB
static void
bus_db_instance_tree_leader(struct bus_instance *parent,
			    struct bus_instance *bi, bool last)
{
	if (parent == NULL)
		return;
	bus_db_instance_tree_leader(parent->bi_parent, parent, false);
	if (last && STAILQ_NEXT(bi, bi_peer) == NULL) {
		kcprintf("`-");
		return;
	}
	if (STAILQ_NEXT(bi, bi_peer) == NULL) {
		kcprintf("  ");
	} else {
		kcprintf("|%c", last ? '-' : ' ');
	}
}

static void
bus_db_instance_tree(struct bus_instance *bi)
{
	struct bus *bus = bi->bi_attachment->ba_bus;
	struct bus_instance *child;

	bus_db_instance_tree_leader(bi->bi_parent, bi, true);
	kcprintf("%s\n", bus->bus_name);

	STAILQ_FOREACH(child, &bi->bi_children, bi_peer) {
		bus_db_instance_tree(child);
	}
}

static void
bus_db_instances(void)
{
	if (bus_root == NULL) {
		kcprintf("No busses attached.\n");
		return;
	}
	bus_db_instance_tree(STAILQ_FIRST(&bus_root->bus_instances));
}
DB_COMMAND(instances, bus, bus_db_instances);
#endif

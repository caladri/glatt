#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <ns/service_directory.h>

struct service_directory_entry {
	char sde_name[NS_SERVICE_NAME_LENGTH];
	ipc_port_t sde_port;
	BTREE_NODE(struct service_directory_entry) sde_tree;
};

static BTREE_ROOT(struct service_directory_entry) service_directory;
static struct mutex service_directory_lock;
static struct pool service_directory_entry_pool;

#define	SERVICE_DIRECTORY_LOCK()	mutex_lock(&service_directory_lock)
#define	SERVICE_DIRECTORY_UNLOCK()	mutex_unlock(&service_directory_lock)

void
service_directory_init(void)
{
	int error;

	error = pool_create(&service_directory_entry_pool, "SERVICE DIRECTORY",
			    sizeof (struct service_directory_entry),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	mutex_init(&service_directory_lock, "Service Directory", MUTEX_FLAG_DEFAULT);
	BTREE_INIT_ROOT(&service_directory);
}

int
service_directory_enter(const char *service_name, ipc_port_t port)
{
	struct service_directory_entry *sde, *old, *iter;

	sde = pool_allocate(&service_directory_entry_pool);
	strlcpy(sde->sde_name, service_name, NS_SERVICE_NAME_LENGTH);
	sde->sde_port = port;
	BTREE_INIT(&sde->sde_tree);

	SERVICE_DIRECTORY_LOCK();

	BTREE_FIND(&old, iter, &service_directory, sde_tree,
		   strcmp(sde->sde_name, iter->sde_name) < 0,
		   strcmp(sde->sde_name, iter->sde_name) == 0);

	if (old != NULL) {
		SERVICE_DIRECTORY_UNLOCK();
		pool_free(sde);
		return (ERROR_NOT_FREE);
	}

	BTREE_INSERT(sde, iter, &service_directory, sde_tree,
		     strcmp(sde->sde_name, iter->sde_name) < 0);

	SERVICE_DIRECTORY_UNLOCK();

	return (0);
}

int
service_directory_lookup(const char *service_name, ipc_port_t *portp)
{
	struct service_directory_entry *sde, *iter;

	SERVICE_DIRECTORY_LOCK();

	BTREE_FIND(&sde, iter, &service_directory, sde_tree,
		   strcmp(service_name, iter->sde_name) < 0,
		   strcmp(service_name, iter->sde_name) == 0);

	if (sde == NULL) {
		SERVICE_DIRECTORY_UNLOCK();
		return (ERROR_NOT_FOUND);
	}

	*portp = sde->sde_port;

	SERVICE_DIRECTORY_UNLOCK();

	return (0);
}

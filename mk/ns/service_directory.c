#include <core/types.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/ipc.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/queue.h>
#include <core/scheduler.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <ns/ns.h>
#include <ns/service_directory.h>

struct service_directory_entry {
	char sde_name[NS_SERVICE_NAME_LENGTH];
	ipc_port_t sde_port;
	STAILQ_ENTRY(struct service_directory_entry) sde_link;
};

static STAILQ_HEAD(, struct service_directory_entry) service_directory;
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
	STAILQ_INIT(&service_directory);
}

int
service_directory_enter(const char *service_name, ipc_port_t port)
{
	return (ERROR_NOT_IMPLEMENTED);
}

int
service_directory_lookup(const char *service_name, ipc_port_t *port)
{
	return (ERROR_NOT_IMPLEMENTED);
}

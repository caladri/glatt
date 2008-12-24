#include <core/types.h>
#include <core/error.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/task.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <ipc/task.h>

struct ipc_port_right {
	ipc_port_t ipcpr_port;
	ipc_port_right_t ipcpr_rights;
	BTREE_NODE(struct ipc_port_right) ipcpr_tree;
};

static struct mutex ipc_port_rights_lock;
static struct pool ipc_port_right_pool;

#define	IPC_PORT_RIGHTS_LOCK()		mutex_lock(&ipc_port_rights_lock)
#define	IPC_PORT_RIGHTS_UNLOCK()	mutex_unlock(&ipc_port_rights_lock)

static struct ipc_port_right *ipc_task_get_port_right(struct ipc_task *,
						      ipc_port_t, bool);

void
ipc_task_init(void)
{
	int error;

	error = pool_create(&ipc_port_right_pool, "IPC PORT RIGHT",
			    sizeof (struct ipc_port_right), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	mutex_init(&ipc_port_rights_lock, "IPC PORT RIGHTS",
		   MUTEX_FLAG_DEFAULT);
}

/*
 * XXX
 *
 * Needs task lock.
 *
 * Must be called while "port" is locked.
 */
int
ipc_task_check_port_right(struct task *task, ipc_port_right_t rights,
			  ipc_port_t port)
{
	struct ipc_task *ipct = &task->t_ipc;
	struct ipc_port_right *ipcpr;

	rights &= IPC_PORT_RIGHT_RECEIVE | IPC_PORT_RIGHT_SEND;

	ASSERT(rights != 0, "Must be requesting a right.");

	/*
	 * Always grant a send-right to the reserved ports.  Perhaps this
	 * ought to be opt-in.
	 */
	if (port > IPC_PORT_UNKNOWN && port < IPC_PORT_UNRESERVED_START) {
		if ((rights & IPC_PORT_RIGHT_SEND) != 0) {
			rights &= ~IPC_PORT_RIGHT_SEND;
			if (rights == IPC_PORT_RIGHT_NONE)
				return (0);
		}
	}

	IPC_PORT_RIGHTS_LOCK();
	ipcpr = ipc_task_get_port_right(ipct, port, false);
	if (ipcpr == NULL) {
		IPC_PORT_RIGHTS_UNLOCK();
		return (ERROR_NO_RIGHT);
	}

	/* Grant any rights explicitly enumerated.  */
	rights &= ~ipcpr->ipcpr_rights;

	/* Grant a one-time send right and then revoke it.  */
	if ((rights & IPC_PORT_RIGHT_SEND) != 0 &&
	    (ipcpr->ipcpr_rights & IPC_PORT_RIGHT_SEND_ONCE) != 0) {
		/* XXX If this send fails restore the right?  */
		rights &= ~IPC_PORT_RIGHT_SEND;
		ipcpr->ipcpr_rights &= ~IPC_PORT_RIGHT_SEND_ONCE;
	}

	IPC_PORT_RIGHTS_UNLOCK();

	/* If there are any remaining rights which were not granted, fail.  */
	if (rights != IPC_PORT_RIGHT_NONE) {
		return (ERROR_NO_RIGHT);
	}

	return (0);

}

/*
 * XXX
 *
 * Needs task lock.
 *
 * Must be called while "port" is locked.
 */
int
ipc_task_insert_port_right(struct task *task, ipc_port_right_t rights,
			   ipc_port_t port)
{
	struct ipc_task *ipct = &task->t_ipc;
	struct ipc_port_right *ipcpr;

	if (rights == 0) {
		return (0);
	}

	IPC_PORT_RIGHTS_LOCK();
	ipcpr = ipc_task_get_port_right(ipct, port, true);
	if (ipcpr == NULL) {
		IPC_PORT_RIGHTS_UNLOCK();
		return (ERROR_EXHAUSTED);
	}

	ipcpr->ipcpr_rights |= rights;
	if ((ipcpr->ipcpr_rights & IPC_PORT_RIGHT_SEND) != 0)
		ipcpr->ipcpr_rights &= ~IPC_PORT_RIGHT_SEND_ONCE;

	IPC_PORT_RIGHTS_UNLOCK();

	return (0);
}

int
ipc_task_setup(struct task *task)
{
	struct ipc_task *ipct = &task->t_ipc;

	BTREE_ROOT_INIT(&ipct->ipct_rights);

	/* Insert any default rights.  */

	return (0);
}

static struct ipc_port_right *
ipc_task_get_port_right(struct ipc_task *ipct, ipc_port_t port, bool create)
{
	struct ipc_port_right *ipcpr, *iter;

	BTREE_FIND(&ipcpr, iter, &ipct->ipct_rights, ipcpr_tree,
		   (port < iter->ipcpr_port), (port == iter->ipcpr_port));
	if (ipcpr == NULL) {
		if (!create) {
			return (NULL);
		}
		ipcpr = pool_allocate(&ipc_port_right_pool);
		ipcpr->ipcpr_port = port;
		ipcpr->ipcpr_rights = IPC_PORT_RIGHT_NONE;
		BTREE_NODE_INIT(&ipcpr->ipcpr_tree);

		BTREE_INSERT(ipcpr, iter, &ipct->ipct_rights, ipcpr_tree,
			     (ipcpr->ipcpr_port < iter->ipcpr_port));
		return (ipcpr);
	}
	return (ipcpr);
}

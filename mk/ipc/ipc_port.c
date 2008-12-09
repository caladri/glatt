#include <core/types.h>
#include <core/btree.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/task.h>
#include <core/thread.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <vm/page.h>

struct ipc_message;
struct ipc_port;
struct ipc_port_right;

struct ipc_message {
	struct ipc_header ipcmsg_header;
	TAILQ_ENTRY(struct ipc_message) ipcmsg_link;
};

struct ipc_port {
	struct mutex ipcp_mutex;
	BTREE_ROOT(struct ipc_port_right) ipcp_rights;
	struct cv *ipcp_cv;
	ipc_port_t ipcp_port;
	TAILQ_HEAD(, struct ipc_message) ipcp_msgs;
	BTREE_NODE(struct ipc_port) ipcp_link;
};

struct ipc_port_right {
	struct task *ipcpr_task;
	ipc_port_right_t ipcpr_rights;
	BTREE_NODE(struct ipc_port_right) ipcpr_tree;
};

static BTREE_ROOT(struct ipc_port) ipc_ports = BTREE_ROOT_INITIALIZER();
static struct mutex ipc_ports_lock;
static struct pool ipc_port_pool;
static struct pool ipc_port_right_pool;
static ipc_port_t ipc_port_next		= IPC_PORT_UNRESERVED_START;

#define	IPC_PORTS_LOCK()	mutex_lock(&ipc_ports_lock)
#define	IPC_PORTS_UNLOCK()	mutex_unlock(&ipc_ports_lock)
#define	IPC_PORT_LOCK(p)	mutex_lock(&(p)->ipcp_mutex)
#define	IPC_PORT_UNLOCK(p)	mutex_unlock(&(p)->ipcp_mutex)

static struct ipc_port *ipc_port_alloc(struct task *, ipc_port_t);
static void ipc_port_deliver(struct ipc_message *);
static struct ipc_port *ipc_port_lookup(ipc_port_t);
static int ipc_port_right_insert(struct ipc_port *, ipc_port_right_t,
				 struct task *);

void
ipc_port_init(void)
{
	int error;

	error = pool_create(&ipc_port_pool, "IPC PORT",
			    sizeof (struct ipc_port),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	error = pool_create(&ipc_port_right_pool, "IPC PORT RIGHT",
			    sizeof (struct ipc_port_right),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	mutex_init(&ipc_ports_lock, "IPC Ports", MUTEX_FLAG_DEFAULT);
}

int
ipc_port_allocate(struct task *task, ipc_port_t *portp)
{
	struct ipc_port *ipcp;
	ipc_port_t port;

	for (;;) {
		IPC_PORTS_LOCK();
		for (;;) {
			if (ipc_port_next < IPC_PORT_UNRESERVED_START) {
				ipc_port_next = IPC_PORT_UNRESERVED_START;
				continue;
			}
			port = ipc_port_next++;
			ipcp = ipc_port_lookup(port);
			if (ipcp == NULL)
				break;
			IPC_PORT_UNLOCK(ipcp);
		}
		IPC_PORTS_UNLOCK();

		ipcp = ipc_port_alloc(task, port);
		if (ipcp == NULL)
			continue;

		*portp = port;
		return (0);
	}
}

int
ipc_port_allocate_reserved(struct task *task, ipc_port_t port)
{
	struct ipc_port *ipcp;

	if (port == IPC_PORT_UNKNOWN || port >= IPC_PORT_UNRESERVED_START)
		return (ERROR_INVALID);

	ipcp = ipc_port_alloc(task, port);
	if (ipcp == NULL)
		return (ERROR_NOT_FREE);
	return (0);
}

void
ipc_port_free(ipc_port_t port)
{
	panic("%s: not implemented yet.", __func__);
}

int
ipc_port_receive(ipc_port_t port, struct ipc_header *ipch)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;

	ASSERT(ipch != NULL, "Must be able to copy out header.");

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}
	/* XXX Check for receive right.  */
	if (TAILQ_EMPTY(&ipcp->ipcp_msgs)) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (ERROR_AGAIN);
	}
	ipcmsg = TAILQ_FIRST(&ipcp->ipcp_msgs);
	ASSERT(ipcmsg != NULL, "Queue must not change out from under us.");
	ASSERT(ipcmsg->ipcmsg_header.ipchdr_dst == ipcp->ipcp_port,
	       "Destination must be this port.");
	TAILQ_REMOVE(&ipcp->ipcp_msgs, ipcmsg, ipcmsg_link);
	IPC_PORT_UNLOCK(ipcp);
	IPC_PORTS_UNLOCK();

	*ipch = ipcmsg->ipcmsg_header;
	free(ipcmsg);
	return (0);
}

int
ipc_port_send(struct ipc_header *ipch)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;

	ASSERT(ipch != NULL, "Must have a header.");

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(ipch->ipchdr_src);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_INVALID);
	}
	/* XXX Check receive and send rights.  */
	IPC_PORT_UNLOCK(ipcp);

	ipcp = ipc_port_lookup(ipch->ipchdr_dst);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}
	IPC_PORT_UNLOCK(ipcp);
	IPC_PORTS_UNLOCK();

	ipcmsg = malloc(sizeof *ipcmsg);
	ipcmsg->ipcmsg_header = *ipch;
	ipc_port_deliver(ipcmsg);
	return (0);
}

void
ipc_port_wait(ipc_port_t port)
{
	struct ipc_port *ipcp;

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	ASSERT(ipcp != NULL, "Must have a port to wait on.");
	if (!TAILQ_EMPTY(&ipcp->ipcp_msgs)) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return;
	}
	/* XXX refcount.  */
	IPC_PORTS_UNLOCK();
	cv_wait(ipcp->ipcp_cv);
}

static struct ipc_port *
ipc_port_alloc(struct task *task, ipc_port_t port)
{
	struct ipc_port *ipcp, *old, *iter;
	int error;

	IPC_PORTS_LOCK();
	old = ipc_port_lookup(port);
	if (old != NULL) {
		IPC_PORT_UNLOCK(old);
		IPC_PORTS_UNLOCK();
		return (NULL);
	}

	ipcp = pool_allocate(&ipc_port_pool);
	mutex_init(&ipcp->ipcp_mutex, "IPC Port", MUTEX_FLAG_DEFAULT);
	ipcp->ipcp_cv = cv_create(&ipcp->ipcp_mutex);
	ipcp->ipcp_port = port;
	TAILQ_INIT(&ipcp->ipcp_msgs);
	BTREE_INIT_ROOT(&ipcp->ipcp_rights);

	IPC_PORT_LOCK(ipcp);
	/*
	 * Check to see if we lost a race for this port.
	 */
	old = ipc_port_lookup(port);
	if (old != NULL) {
		IPC_PORT_UNLOCK(old);
		pool_free(ipcp);
		IPC_PORTS_UNLOCK();
		return (NULL);
	}

	/*
	 * Insert a receive right.
	 */
	error = ipc_port_right_insert(ipcp, IPC_PORT_RIGHT_RECEIVE, task);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		pool_free(ipcp);
		IPC_PORTS_UNLOCK();
		return (NULL);
	}

	/*
	 * Register ourselves.
	 */
	BTREE_INSERT(ipcp, iter, &ipc_ports, ipcp_link,
		     (ipcp->ipcp_port < iter->ipcp_port));

	IPC_PORT_UNLOCK(ipcp);

	/* XXX */
	old = ipc_port_lookup(port);
	if (old != ipcp)
		panic("%s: port we just allocated is duplicate.\n", __func__);
	IPC_PORT_UNLOCK(old);
	IPC_PORTS_UNLOCK();

	return (ipcp);
}

static void
ipc_port_deliver(struct ipc_message *ipcmsg)
{
	struct ipc_port *ipcp;

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(ipcmsg->ipcmsg_header.ipchdr_dst);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		free(ipcmsg);
		return;
	}
	ASSERT(ipcmsg->ipcmsg_header.ipchdr_dst == ipcp->ipcp_port,
	       "Destination must be the same as the port we are inserting into.");
	TAILQ_INSERT_TAIL(&ipcp->ipcp_msgs, ipcmsg, ipcmsg_link);
	cv_signal(ipcp->ipcp_cv);
	IPC_PORT_UNLOCK(ipcp);
	IPC_PORTS_UNLOCK();
}

static struct ipc_port *
ipc_port_lookup(ipc_port_t port)
{
	struct ipc_port *ipcp, *iter;

	BTREE_FIND(&ipcp, iter, &ipc_ports, ipcp_link, (port < iter->ipcp_port),
		   (port == iter->ipcp_port));
	if (ipcp != NULL) {
		IPC_PORT_LOCK(ipcp);
		return (ipcp);
	}
	return (NULL);
}

static int
ipc_port_right_insert(struct ipc_port *ipcp, ipc_port_right_t rights,
		      struct task *task)
{
	struct ipc_port_right *ipcpr, *iter;

	if ((rights & IPC_PORT_RIGHT_SEND) != 0)
		rights &= ~IPC_PORT_RIGHT_SEND_ONCE;

	BTREE_FIND(&ipcpr, iter, &ipcp->ipcp_rights, ipcpr_tree,
		   (task < iter->ipcpr_task), (task == iter->ipcpr_task));
	if (ipcpr != NULL) {
		ipcpr->ipcpr_rights |= rights;
		if ((ipcpr->ipcpr_rights & IPC_PORT_RIGHT_SEND) != 0)
			ipcpr->ipcpr_rights &= ~IPC_PORT_RIGHT_SEND_ONCE;
		return (0);
	}

	ipcpr = pool_allocate(&ipc_port_right_pool);
	ipcpr->ipcpr_task = task;
	ipcpr->ipcpr_rights = rights;
	BTREE_INIT(&ipcpr->ipcpr_tree);

	BTREE_INSERT(ipcpr, iter, &ipcp->ipcp_rights, ipcpr_tree,
		     (ipcpr->ipcpr_task < iter->ipcpr_task));

	return (0);
}

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
#include <ipc/data.h>
#include <ipc/ipc.h>
#include <ipc/port.h>

struct ipc_message;
struct ipc_port;

struct ipc_message {
	struct ipc_header ipcmsg_header;
	struct ipc_data *ipcmsg_data;
	TAILQ_ENTRY(struct ipc_message) ipcmsg_link;
};

struct ipc_port {
	struct mutex ipcp_mutex;
	struct cv *ipcp_cv;
	ipc_port_t ipcp_port;
	TAILQ_HEAD(, struct ipc_message) ipcp_msgs;
	BTREE_NODE(struct ipc_port) ipcp_link;
	/* XXX Need a list of tasks which are holding rights and a refcnt.  */
};

static BTREE_ROOT(struct ipc_port) ipc_ports = BTREE_ROOT_INITIALIZER();
static struct mutex ipc_ports_lock;
static struct pool ipc_port_pool;
static ipc_port_t ipc_port_next		= IPC_PORT_UNRESERVED_START;

#define	IPC_PORTS_LOCK()	mutex_lock(&ipc_ports_lock)
#define	IPC_PORTS_UNLOCK()	mutex_unlock(&ipc_ports_lock)
#define	IPC_PORT_LOCK(p)	mutex_lock(&(p)->ipcp_mutex)
#define	IPC_PORT_UNLOCK(p)	mutex_unlock(&(p)->ipcp_mutex)

static struct ipc_port *ipc_port_alloc(void);
static struct ipc_port *ipc_port_lookup(ipc_port_t);
static int ipc_port_register(struct task *, struct ipc_port *, ipc_port_t);

void
ipc_port_init(void)
{
	int error;

	error = pool_create(&ipc_port_pool, "IPC PORT",
			    sizeof (struct ipc_port),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	mutex_init(&ipc_ports_lock, "IPC Ports", MUTEX_FLAG_DEFAULT);
}

int
ipc_port_allocate(struct task *task, ipc_port_t *portp)
{
	struct ipc_port *ipcp, *old;
	ipc_port_t port;
	int error;

	ipcp = ipc_port_alloc();
	if (ipcp == NULL)
		return (ERROR_EXHAUSTED);

	IPC_PORTS_LOCK();
	for (;;) {
		if (ipc_port_next < IPC_PORT_UNRESERVED_START) {
			ipc_port_next = IPC_PORT_UNRESERVED_START;
			continue;
		}
		port = ipc_port_next++;

		old = ipc_port_lookup(port);
		if (old != NULL) {
			IPC_PORT_UNLOCK(old);
			continue;
		}

		IPC_PORT_LOCK(ipcp);
		error = ipc_port_register(task, ipcp, port);
		if (error != 0)
			panic("%s: ipc_port_register failed: %m", __func__,
			      error);
		IPC_PORT_UNLOCK(ipcp);

		IPC_PORTS_UNLOCK();

		*portp = port;

		return (0);
	}
}

int
ipc_port_allocate_reserved(struct task *task, ipc_port_t port)
{
	struct ipc_port *ipcp, *old;
	int error;

	if (port == IPC_PORT_UNKNOWN || port >= IPC_PORT_UNRESERVED_START)
		return (ERROR_INVALID);

	IPC_PORTS_LOCK();

	old = ipc_port_lookup(port);
	if (old != NULL) {
		IPC_PORT_UNLOCK(old);
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FREE);
	}

	ipcp = ipc_port_alloc();
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_EXHAUSTED);
	}

	IPC_PORT_LOCK(ipcp);
	error = ipc_port_register(task, ipcp, port);
	if (error != 0)
		panic("%s: ipc_port_register failed: %m", __func__, error);
	IPC_PORT_UNLOCK(ipcp);

	IPC_PORTS_UNLOCK();

	return (0);
}

int
ipc_port_receive(ipc_port_t port, struct ipc_header *ipch,
		 struct ipc_data **ipcdp)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;
	int error;

	ASSERT(ipch != NULL, "Must be able to copy out header.");

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}

	error = ipc_task_check_port_right(current_task(),
					  IPC_PORT_RIGHT_RECEIVE, port);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (error);
	}

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

	/*
	 * Insert any passed rights.
	 */
	if (ipcmsg->ipcmsg_header.ipchdr_right != IPC_PORT_RIGHT_NONE) {
		ipcp = ipc_port_lookup(ipcmsg->ipcmsg_header.ipchdr_src);
		if (ipcp == NULL)
			panic("%s: port disappeared.", __func__);
		error = ipc_task_insert_port_right(current_task(),
						   ipcmsg->ipcmsg_header.ipchdr_right,
						   ipcmsg->ipcmsg_header.ipchdr_src);
		if (error != 0)
			panic("%s: grating rights failed: %m", __func__,
			      error);
		IPC_PORT_UNLOCK(ipcp);
	}

	IPC_PORTS_UNLOCK();

	*ipch = ipcmsg->ipcmsg_header;
	if (ipcdp != NULL) {
		*ipcdp = ipcmsg->ipcmsg_data;
	} else {
		if (ipcmsg->ipcmsg_data != NULL) {
			ipc_data_free(ipcmsg->ipcmsg_data);
		}
	}
	ipcmsg->ipcmsg_data = NULL;

	free(ipcmsg);

	return (0);
}

int
ipc_port_send(struct ipc_header *ipch, struct ipc_data *ipcd)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;
	int error;

	ASSERT(ipch != NULL, "Must have a header.");

	IPC_PORTS_LOCK();

	/*
	 * Step 1:
	 * Check that the sending task has a receive right on the source port.
	 */
	ipcp = ipc_port_lookup(ipch->ipchdr_src);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_INVALID);
	}

	error = ipc_task_check_port_right(current_task(),
					  IPC_PORT_RIGHT_RECEIVE,
					  ipch->ipchdr_src);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (error);
	}
	IPC_PORT_UNLOCK(ipcp);

	/*
	 * Step 2:
	 * Check that the sending task has a send right on the destination port.
	 */
	ipcp = ipc_port_lookup(ipch->ipchdr_dst);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}

	error = ipc_task_check_port_right(current_task(),
					  IPC_PORT_RIGHT_SEND,
					  ipch->ipchdr_dst);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (error);
	}

	ipcmsg = malloc(sizeof *ipcmsg);
	ipcmsg->ipcmsg_header = *ipch;
	ipcmsg->ipcmsg_data = NULL;

	if (ipcd != NULL) {
		error = ipc_data_copyin(ipcd, &ipcmsg->ipcmsg_data);
		if (error != 0) {
			IPC_PORT_UNLOCK(ipcp);
			IPC_PORTS_UNLOCK();
			free(ipcmsg);
			return (error);
		}
	}

	TAILQ_INSERT_TAIL(&ipcp->ipcp_msgs, ipcmsg, ipcmsg_link);
	cv_signal(ipcp->ipcp_cv);

	IPC_PORT_UNLOCK(ipcp);
	IPC_PORTS_UNLOCK();

	return (0);
}

int
ipc_port_wait(ipc_port_t port)
{
	struct ipc_port *ipcp;
	int error;

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	ASSERT(ipcp != NULL, "Must have a port to wait on.");
	if (!TAILQ_EMPTY(&ipcp->ipcp_msgs)) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (0);
	}

	error = ipc_task_check_port_right(current_task(),
					  IPC_PORT_RIGHT_RECEIVE, port);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (error);
	}

	/* XXX refcount.  */

	IPC_PORTS_UNLOCK();
	cv_wait(ipcp->ipcp_cv);

	return (0);
}

static struct ipc_port *
ipc_port_alloc(void)
{
	struct ipc_port *ipcp;

	ipcp = pool_allocate(&ipc_port_pool);
	mutex_init(&ipcp->ipcp_mutex, "IPC Port", MUTEX_FLAG_DEFAULT);
	ipcp->ipcp_cv = cv_create(&ipcp->ipcp_mutex);
	ipcp->ipcp_port = IPC_PORT_UNKNOWN;
	TAILQ_INIT(&ipcp->ipcp_msgs);
	BTREE_NODE_INIT(&ipcp->ipcp_link);

	return (ipcp);
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
ipc_port_register(struct task *task, struct ipc_port *ipcp, ipc_port_t port)
{
	struct ipc_port *old, *iter;
	int error;

	old = ipc_port_lookup(port);
	if (old != NULL) {
		IPC_PORT_UNLOCK(old);
		return (ERROR_NOT_FREE);
	}

	ASSERT(ipcp->ipcp_port == IPC_PORT_UNKNOWN, "Can't reuse ipc_port.");
	ipcp->ipcp_port = port;

	/*
	 * Register ourselves.
	 */
	BTREE_INSERT(ipcp, iter, &ipc_ports, ipcp_link,
		     (ipcp->ipcp_port < iter->ipcp_port));

	/*
	 * Insert a receive right.
	 */
	error = ipc_task_insert_port_right(task, IPC_PORT_RIGHT_RECEIVE,
					   ipcp->ipcp_port);
	if (error != 0)
		panic("%s: ipc_task_insert_port_right failed: %m", __func__,
		      error);

	return (0);
}

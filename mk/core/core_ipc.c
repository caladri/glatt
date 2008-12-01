#include <core/types.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/ipc.h>
#include <core/malloc.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/thread.h>
#include <vm/page.h>

struct ipc_port {
	struct mutex ipcp_mutex;
	struct cv *ipcp_cv;
	ipc_port_t ipcp_port;
	TAILQ_HEAD(, struct ipc_message) ipcp_msgs;
	TAILQ_ENTRY(struct ipc_port) ipcp_link;
};

struct ipc_message {
	struct ipc_header ipcmsg_header;
	vaddr_t ipcmsg_page;
	TAILQ_ENTRY(struct ipc_message) ipcmsg_link;
};

static TAILQ_HEAD(, struct ipc_port) ipc_ports;
static struct mutex ipc_ports_lock;
static struct pool ipc_port_pool;
static ipc_port_t ipc_port_next		= IPC_PORT_UNRESERVED_START;

#define	IPC_PORTS_LOCK()	mutex_lock(&ipc_ports_lock)
#define	IPC_PORTS_UNLOCK()	mutex_unlock(&ipc_ports_lock)
#define	IPC_PORT_LOCK(p)	mutex_lock(&(p)->ipcp_mutex)
#define	IPC_PORT_UNLOCK(p)	mutex_unlock(&(p)->ipcp_mutex)

static struct ipc_port *ipc_port_alloc(ipc_port_t);
static void ipc_port_deliver(struct ipc_message *);
static struct ipc_port *ipc_port_lookup(ipc_port_t);

void
ipc_init(void)
{
	int error;

	error = pool_create(&ipc_port_pool, "IPC PORT",
			    sizeof (struct ipc_port),
			    POOL_DEFAULT | POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);

	mutex_init(&ipc_ports_lock, "IPC Ports", MUTEX_FLAG_DEFAULT);
	TAILQ_INIT(&ipc_ports);
}

int
ipc_send(struct ipc_header *ipch, vaddr_t *pagep)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;

	ASSERT(ipch != NULL, "Must have a header.");

	if (pagep != NULL)
		return (ERROR_NOT_IMPLEMENTED);

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(ipch->ipchdr_src);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_INVALID);
	}
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
	ipcmsg->ipcmsg_page = (vaddr_t)NULL;
	ipc_port_deliver(ipcmsg);
	return (0);
}

int
ipc_port_allocate(ipc_port_t *portp)
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

		ipcp = ipc_port_alloc(port);
		if (ipcp == NULL)
			continue;
		*portp = port;
		return (0);
	}
}

int
ipc_port_allocate_reserved(ipc_port_t *portp, ipc_port_t port)
{
	struct ipc_port *ipcp;

	if (port == IPC_PORT_UNKNOWN || port >= IPC_PORT_UNRESERVED_START)
		return (ERROR_INVALID);

	ipcp = ipc_port_alloc(port);
	if (ipcp == NULL)
		return (ERROR_NOT_FREE);
	*portp = port;
	return (0);
}

void
ipc_port_free(ipc_port_t port)
{
	panic("%s: not implemented yet.", __func__);
}

int
ipc_port_receive(ipc_port_t port, struct ipc_header *ipch, vaddr_t *pagep)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;

	ASSERT(ipch != NULL, "Must be able to copy out header.");

	if (pagep != NULL)
		return (ERROR_NOT_IMPLEMENTED);

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
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
	IPC_PORTS_UNLOCK();

	*ipch = ipcmsg->ipcmsg_header;
	free(ipcmsg);
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
ipc_port_alloc(ipc_port_t port)
{
	struct ipc_port *ipcp, *old;

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
	 * Register ourselves.
	 */
	TAILQ_INSERT_TAIL(&ipc_ports, ipcp, ipcp_link);

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
	struct ipc_port *ipcp;

	if (port == IPC_PORT_UNKNOWN)
		return (NULL);

	/*
	 * XXX
	 * System ports will be frequently used so they should be in a static
	 * array.
	 */

	TAILQ_FOREACH(ipcp, &ipc_ports, ipcp_link) {
		IPC_PORT_LOCK(ipcp);
		if (ipcp->ipcp_port == port) {
			return (ipcp);
		}
		IPC_PORT_UNLOCK(ipcp);
	}
	return (NULL);
}

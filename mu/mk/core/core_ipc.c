#include <core/types.h>
#include <core/error.h>
#include <core/ipc.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/sleepq.h>
#include <core/startup.h>
#include <core/thread.h>

struct ipc_port {
	struct mutex ipcp_mutex;
	ipc_port_t ipcp_port;
	TAILQ_HEAD(, struct ipc_message) ipcp_msgs;
	TAILQ_ENTRY(struct ipc_port) ipcp_link;
};

struct ipc_message {
	struct ipc_header ipcmsg_header;
	struct ipc_data ipcmsg_data;
	TAILQ_ENTRY(struct ipc_message) ipcmsg_link;
};

struct ipc_queue {
	struct mutex ipcq_mutex;
	TAILQ_ENTRY(struct ipc_queue) ipcq_link;
	TAILQ_HEAD(, struct ipc_message) ipcq_msgs;
};

static TAILQ_HEAD(, struct ipc_port) ipc_ports;
static struct mutex ipc_ports_lock;
static struct pool ipc_port_pool;
static ipc_port_t ipc_port_next		= IPC_PORT_UNRESERVED_START;

#define	IPC_PORTS_LOCK()	mutex_lock(&ipc_ports_lock)
#define	IPC_PORTS_UNLOCK()	mutex_unlock(&ipc_ports_lock)
#define	IPC_PORT_LOCK(p)	mutex_lock(&(p)->ipcp_mutex)
#define	IPC_PORT_UNLOCK(p)	mutex_unlock(&(p)->ipcp_mutex)

static TAILQ_HEAD(, struct ipc_queue) ipc_queues;
static struct mutex ipc_queues_lock;

#define	IPC_QUEUES_LOCK()	mutex_lock(&ipc_queues_lock)
#define	IPC_QUEUES_UNLOCK()	mutex_unlock(&ipc_queues_lock)
#define	IPC_QUEUE_LOCK(q)	mutex_lock(&(q)->ipcq_mutex)
#define	IPC_QUEUE_UNLOCK(q)	mutex_unlock(&(q)->ipcq_mutex)

#define	current_ipcq()		(PCPU_GET(ipc_queue))

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

	mutex_init(&ipc_ports_lock, "IPC Ports");
	TAILQ_INIT(&ipc_ports);

	mutex_init(&ipc_queues_lock, "IPC Queues");
	TAILQ_INIT(&ipc_queues);
}

void
ipc_process(void)
{
	struct ipc_queue *ipcq;

	ipcq = current_ipcq();

	IPC_QUEUES_LOCK();
	IPC_QUEUE_LOCK(ipcq);
	TAILQ_INSERT_TAIL(&ipc_queues, ipcq, ipcq_link);
	IPC_QUEUE_UNLOCK(ipcq);
	IPC_QUEUES_UNLOCK();

	for (;;) {
		struct ipc_message *ipcmsg;

		IPC_QUEUE_LOCK(ipcq);
		if (TAILQ_EMPTY(&ipcq->ipcq_msgs)) {
			ASSERT(TAILQ_EMPTY(&ipcq->ipcq_msgs), "Consistency is all I ask.");
			sleepq_enter(ipcq);
			IPC_QUEUE_UNLOCK(ipcq);
			sleepq_wait();
			continue;
		}
		ipcmsg = TAILQ_FIRST(&ipcq->ipcq_msgs);
		TAILQ_REMOVE(&ipcq->ipcq_msgs, ipcmsg, ipcmsg_link);
		IPC_QUEUE_UNLOCK(ipcq);
		ipc_port_deliver(ipcmsg);
		IPC_QUEUE_LOCK(ipcq);
		IPC_QUEUE_UNLOCK(ipcq);
	}
}

int
ipc_send(struct ipc_header *ipch, struct ipc_data *ipcd)
{
	struct ipc_message *ipcmsg;
	struct ipc_queue *ipcq;
	struct ipc_port *ipcp;

	ASSERT(ipch != NULL, "Must have a header.");
	ASSERT(ipch->ipchdr_len == 0 || ipcd != NULL,
	       "Must have data or no size.");

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
	if (ipcd != NULL)
		ipcmsg->ipcmsg_data = *ipcd;

	/*
	 * XXX
	 * This should be in ipc_queue_msg or something.
	 */
	ipcq = current_ipcq();
	ASSERT(ipcq != NULL, "Queue must exist.");
	IPC_QUEUE_LOCK(ipcq);
	TAILQ_INSERT_TAIL(&ipcq->ipcq_msgs, ipcmsg, ipcmsg_link);
	sleepq_signal_one(ipcq);
	IPC_QUEUE_UNLOCK(ipcq);
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
			if (ipc_port_next < IPC_PORT_UNRESERVED_START ||
			    ipc_port_next >= IPC_PORT_UNRESERVED_END) {
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

void
ipc_port_free(ipc_port_t port)
{
	panic("%s: not implemented yet.", __func__);
}

int
ipc_port_receive(ipc_port_t port, struct ipc_header *ipch, struct ipc_data *ipcd)
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
	if (ipcd != NULL) {
		*ipcd = ipcmsg->ipcmsg_data;
	}
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
	sleepq_enter(ipcp);
	IPC_PORT_UNLOCK(ipcp);
	IPC_PORTS_UNLOCK();
	sleepq_wait();
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
	mutex_init(&ipcp->ipcp_mutex, "IPC Port");
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
	sleepq_signal_one(ipcp);
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

static void
ipc_queue_startup(void *arg)
{
	struct ipc_queue *ipcq;

	/*
	 * Must not block.
	 */
	ipcq = malloc(sizeof *ipcq);
	mutex_init(&ipcq->ipcq_mutex, "IPC Queue");
	PCPU_SET(ipc_queue, ipcq);

	ipcq = current_ipcq();

	TAILQ_INIT(&ipcq->ipcq_msgs);
}
STARTUP_ITEM(ipc_queue, STARTUP_IPCQ, STARTUP_CPU, ipc_queue_startup, NULL);

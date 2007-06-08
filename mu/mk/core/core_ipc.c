#include <core/types.h>
#include <core/ipc.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/sleepq.h>
#include <core/thread.h>
#include <core/vdae.h>

static TAILQ_HEAD(, struct ipc_port) ipc_ports;
static struct mutex ipc_ports_lock;
static struct pool ipc_port_pool;

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

#define	current_ipcq()		(&PCPU_GET(ipc_queue))

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

/*
 * Set up a queue for this CPU and process everything in it ad absurdum.
 */
void
ipc_process(void)
{
	struct ipc_queue *ipcq;

	ipcq = current_ipcq();

	mutex_init(&ipcq->ipcq_mutex, "IPC Queue");
	TAILQ_INIT(&ipcq->ipcq_msgs);
	IPC_QUEUES_LOCK();
	IPC_QUEUE_LOCK(ipcq);
	TAILQ_INSERT_TAIL(&ipc_queues, ipcq, ipcq_link);
	IPC_QUEUE_UNLOCK(ipcq);
	IPC_QUEUES_UNLOCK();

	for (;;) {
		struct ipc_message *ipcmsg;

		IPC_QUEUE_LOCK(ipcq);
		if (TAILQ_EMPTY(&ipcq->ipcq_msgs)) {
			IPC_QUEUE_UNLOCK(ipcq);
			sleepq_wait(ipcq);
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

static struct ipc_port *
ipc_port_alloc(ipc_port_t port)
{
	struct ipc_port *ipcp, *old;

	old = ipc_port_lookup(port);
	if (old != NULL) {
		IPC_PORT_UNLOCK(old);
		return (NULL);
	}

	ipcp = pool_allocate(&ipc_port_pool);
	mutex_init(&ipcp->ipcp_mutex, "IPC Port");
	ipcp->ipcp_port = port;
	ipcp->ipcp_vdae = NULL;
	TAILQ_INIT(&ipcp->ipcp_msgs);

	IPC_PORTS_LOCK();
	IPC_PORT_LOCK(ipcp);
	/*
	 * Check to see if we lost a race for this port.
	 */
	old = ipc_port_lookup(port);
	if (old != NULL) {
		IPC_PORT_UNLOCK(old);
		pool_free(ipcp);
		return (NULL);
	}
	IPC_PORT_UNLOCK(ipcp);
	IPC_PORTS_UNLOCK();
}

static void
ipc_port_deliver(struct ipc_message *ipcmsg)
{
	struct ipc_port *ipcp;

	ipcp = ipc_port_lookup(ipcmsg->ipcmsg_header.ipchdr_dst);
	if (ipcp != NULL) {
		TAILQ_INSERT_TAIL(&ipcp->ipcp_msgs, ipcmsg, ipcmsg_link);
		vdae_list_wakeup(ipcp->ipcp_vdae);
		IPC_PORT_UNLOCK(ipcp);
	}
	free(ipcmsg);
}

static struct ipc_port *
ipc_port_lookup(ipc_port_t port)
{
	struct ipc_port *ipcp;

	IPC_PORTS_LOCK();
	TAILQ_FOREACH(ipcp, &ipc_ports, ipcp_link) {
		IPC_PORT_LOCK(ipcp);
		if (ipcp->ipcp_port == port) {
			IPC_PORTS_UNLOCK();
			return (ipcp);
		}
		IPC_PORT_UNLOCK(ipcp);
	}
	IPC_PORTS_UNLOCK();
	return (NULL);
}

#include <core/types.h>
#include <core/btree.h>
#include <core/cv.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/startup.h>
#include <core/string.h>
#include <core/task.h>
#include <core/thread.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <vm/vm.h>
#include <vm/vm_index.h>
#include <vm/vm_page.h>

struct ipc_port;

struct ipc_message {
	struct ipc_header ipcmsg_header;
	struct vm_page *ipcmsg_page;
	TAILQ_ENTRY(struct ipc_message) ipcmsg_link;
};

struct ipc_port {
	struct mutex ipcp_mutex;
	struct cv *ipcp_cv;
	ipc_port_t ipcp_port;
	ipc_port_flags_t ipcp_flags;
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
static int ipc_port_register(struct task *, struct ipc_port *, ipc_port_t, ipc_port_flags_t);

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
ipc_port_allocate(struct task *task, ipc_port_t *portp, ipc_port_flags_t flags)
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
		error = ipc_port_register(task, ipcp, port, flags);
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
ipc_port_allocate_reserved(struct task *task, ipc_port_t port, ipc_port_flags_t flags)
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
	error = ipc_port_register(task, ipcp, port, flags);
	if (error != 0)
		panic("%s: ipc_port_register failed: %m", __func__, error);
	IPC_PORT_UNLOCK(ipcp);

	IPC_PORTS_UNLOCK();

	return (0);
}

/*
 * XXX
 * receive could take a task-local port number like a fd and speed lookup and
 * minimize locking.
 */
int
ipc_port_receive(ipc_port_t port, struct ipc_header *ipch, void **vpagep)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;
	struct task *task;
	vaddr_t vaddr;
	int error, error2;

	task = current_task();

	ASSERT(task != NULL, "Must have a running task.");
	ASSERT(ipch != NULL, "Must be able to copy out header.");

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}

	error = ipc_task_check_port_right(task, IPC_PORT_RIGHT_RECEIVE, port);
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
		error = ipc_task_insert_port_right(task,
						   ipcmsg->ipcmsg_header.ipchdr_right,
						   ipcmsg->ipcmsg_header.ipchdr_src);
		if (error != 0)
			panic("%s: grating rights failed: %m", __func__,
			      error);
		IPC_PORT_UNLOCK(ipcp);
	}

	IPC_PORTS_UNLOCK();

	if (ipcmsg->ipcmsg_page == NULL) {
		if (vpagep != NULL)
			*vpagep = NULL;
	} else {
		if (vpagep == NULL) {
			/*
			 * A task may refuse a page flip for any number of reasons.
			 */
			page_release(ipcmsg->ipcmsg_page);
		} else {
			/*
			 * Map this page into the receiving task.
			 */
			if ((task->t_flags & TASK_KERNEL) == 0) {
				/*
				 * User task.
				 */
				error = vm_alloc_address(task->t_vm, &vaddr, 1);
				if (error != 0) {
					page_release(ipcmsg->ipcmsg_page);
					free(ipcmsg);
					return (error);
				}

				error = page_map(task->t_vm, vaddr, ipcmsg->ipcmsg_page);
				if (error != 0) {
					error2 = vm_free_address(task->t_vm, vaddr);
					if (error2 != 0)
						panic("%s: vm_free_address failed: %m", __func__, error);
					page_release(ipcmsg->ipcmsg_page);
					free(ipcmsg);
				}
			} else {
				/*
				 * Kernel task.
				 */
				error = page_map_direct(&kernel_vm, ipcmsg->ipcmsg_page, &vaddr);
				if (error != 0) {
					page_release(ipcmsg->ipcmsg_page);
					free(ipcmsg);
					return (error);
				}
			}
			*vpagep = (void *)vaddr;
		}
	}

	*ipch = ipcmsg->ipcmsg_header;

	free(ipcmsg);

	return (0);
}

int
ipc_port_send(struct ipc_header *ipch, void *vpage)
{
	struct vm_page *page;
	struct task *task;
	struct vm *vm;
	int error;

	task = current_task();

	ASSERT(task != NULL, "Must have a running task.");
	ASSERT(ipch != NULL, "Must have a header.");

	/*
	 * Extract the vm_page for this page.
	 */
	if (vpage == NULL) {
		page = NULL;
	} else {
		if ((task->t_flags & TASK_KERNEL) == 0)
			vm = task->t_vm;
		else
			vm = &kernel_vm;
		error = page_extract(vm, (vaddr_t)vpage, &page);
		if (error != 0)
			return (error);
		error = page_unmap(vm, (vaddr_t)vpage, page);
		if (error != 0)
			panic("%s: could not unmap source page: %m", __func__, error);
		error = vm_free_address(vm, (vaddr_t)vpage);
		if (error != 0)
			panic("%s: could not free source page address: %m", __func__, error);
	}

	error = ipc_port_send_page(ipch, page);
	if (error != 0) {
		if (page != NULL)
			page_release(page);
		return (error);
	}

	return (0);
}

/*
 * NB: Don't check recsize or reclen.
 */
int
ipc_port_send_data(struct ipc_header *ipch, const void *p, size_t len)
{
	struct vm_page *page;
	struct task *task;
	vaddr_t vaddr;
	int error;

	task = current_task();

	ASSERT(task != NULL, "Must have a running task.");

	if (p == NULL) {
		ASSERT(len == 0, "Cannot send no data with a set data length.");
		if (len != 0)
			return (ERROR_INVALID);
		error = ipc_port_send_page(ipch, NULL);
		if (error != 0)
			return (error);
		return (0);
	}

	ASSERT(len != 0, "Cannot send data without data length.");
	ASSERT(len <= PAGE_SIZE, "Cannot send more than a page.");

	error = page_alloc(PAGE_FLAG_DEFAULT, &page);
	if (error != 0)
		return (error);

	error = page_map_direct(&kernel_vm, page, &vaddr);
	if (error != 0) {
		page_release(page);
		return (error);
	}

	memcpy((void *)vaddr, p, len);
	/*
	 * Clear any trailing data so we don't leak kernel information.
	 */
	if (len != PAGE_SIZE)
		memset((void *)(vaddr + len), 0, PAGE_SIZE - len);

	error = page_unmap_direct(&kernel_vm, page, vaddr);
	if (error != 0)
		panic("%s: page_unmap_direct failed: %m", __func__, error);

	error = ipc_port_send_page(ipch, page);
	if (error != 0) {
		page_release(page);
		return (error);
	}

	return (0);
}

int
ipc_port_send_page(struct ipc_header *ipch, struct vm_page *page)
{
	struct ipc_message *ipcmsg;
	struct ipc_port *ipcp;
	struct task *task;
	int error;

	task = current_task();

	ASSERT(task != NULL, "Must have a running task.");
	ASSERT(ipch != NULL, "Must have a header.");

	/*
	 * A message of IPC_MSG_NONE may always be sent to any port by any
	 * port, may not contain any data, and may be used to grant rights.
	 *
	 * XXX There is probably a DoS in allowing rights to be inserted
	 *     for arbitrary tasks, but it's limited to the number of ports,
	 *     so clamping that value for untrusted tasks is probably a fine
	 *     compromise for now.
	 */
	if (ipch->ipchdr_msg == IPC_MSG_NONE && page != NULL)
		return (ERROR_INVALID);

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

	error = ipc_task_check_port_right(task, IPC_PORT_RIGHT_RECEIVE,
					  ipch->ipchdr_src);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (error);
	}
	IPC_PORT_UNLOCK(ipcp);

	/*
	 * Step 2:
	 * Check that the sending task has a send right on the destination port
	 * unless the destination port is providing a public service or a knock
	 * message is being sent.
	 */
	ipcp = ipc_port_lookup(ipch->ipchdr_dst);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}

	if ((ipcp->ipcp_flags & IPC_PORT_FLAG_PUBLIC) == 0 &&
	    ipch->ipchdr_msg != IPC_MSG_NONE) {
		error = ipc_task_check_port_right(task, IPC_PORT_RIGHT_SEND,
						  ipch->ipchdr_dst);
		if (error != 0) {
			IPC_PORT_UNLOCK(ipcp);
			IPC_PORTS_UNLOCK();
			return (error);
		}
	}

	ipcmsg = malloc(sizeof *ipcmsg);
	ipcmsg->ipcmsg_header = *ipch;
	ipcmsg->ipcmsg_page = page;

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
	struct task *task;
	int error;

	task = current_task();

	ASSERT(task != NULL, "Must have a running task.");

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	ASSERT(ipcp != NULL, "Must have a port to wait on.");
	if (!TAILQ_EMPTY(&ipcp->ipcp_msgs)) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (0);
	}

	error = ipc_task_check_port_right(task,
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
	ipcp->ipcp_flags = IPC_PORT_FLAG_NEW;
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
ipc_port_register(struct task *task, struct ipc_port *ipcp, ipc_port_t port, ipc_port_flags_t flags)
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

	ipcp->ipcp_flags &= ~IPC_PORT_FLAG_NEW;
	ipcp->ipcp_flags |= flags;

	return (0);
}

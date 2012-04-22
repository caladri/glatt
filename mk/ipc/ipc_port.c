#include <core/types.h>
#include <core/btree.h>
#include <core/cv.h>
#include <core/console.h>
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

struct ipc_port_right {
	struct task *ipcpr_task;
	ipc_port_right_t ipcpr_right;
	/*
	 * Each port right is in a BTREE by task that lets
	 * rights be looked up for a given task on a port
	 * quickly.  The BTREE_ROOT is in the port itself.
	 */
	BTREE_NODE(struct ipc_port_right) ipcpr_node;
	/*
	 * Each port right is in a STAILQ that lets all
	 * rights associated with a task be enumerated.
	 * The STAILQ_HEAD is in the ipc_task structure.
	 */
	STAILQ_ENTRY(struct ipc_port_right) ipcpr_link;
};

struct ipc_port {
	struct mutex ipcp_mutex;
	struct cv *ipcp_cv;
	ipc_port_t ipcp_port;
	ipc_port_flags_t ipcp_flags;
	TAILQ_HEAD(, struct ipc_message) ipcp_msgs;
	BTREE_NODE(struct ipc_port) ipcp_link;
	/*
	 * XXX
	 * Need a count of receive rights and to expire _all_
	 * rights when the last receive right is dropped.
	 */
	BTREE_ROOT(struct ipc_port_right) ipcp_rights;
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

static struct ipc_port *ipc_port_alloc(void);
static struct ipc_port *ipc_port_lookup(ipc_port_t);
static int ipc_port_register(struct ipc_port *, ipc_port_t, ipc_port_flags_t);

static bool ipc_port_right_check(struct ipc_port *, struct task *, ipc_port_right_t);
static int ipc_port_right_insert(struct ipc_port *, struct task *, ipc_port_right_t);
static struct ipc_port_right *ipc_port_right_lookup(struct ipc_port *, struct task *);
static int ipc_port_right_remove(struct ipc_port *, struct task *, ipc_port_right_t);

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
ipc_port_allocate(ipc_port_t *portp, ipc_port_flags_t flags)
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
		error = ipc_port_register(ipcp, port, flags);
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
ipc_port_allocate_reserved(ipc_port_t port, ipc_port_flags_t flags)
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
	IPC_PORTS_UNLOCK();

	ipcp = ipc_port_alloc();
	if (ipcp == NULL)
		return (ERROR_EXHAUSTED);

	IPC_PORTS_LOCK();
	IPC_PORT_LOCK(ipcp);
	error = ipc_port_register(ipcp, port, flags);
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
	IPC_PORTS_UNLOCK();

	if (!ipc_port_right_check(ipcp, task, IPC_PORT_RIGHT_RECEIVE)) {
		IPC_PORT_UNLOCK(ipcp);
		return (ERROR_NO_RIGHT);
	}

	if (TAILQ_EMPTY(&ipcp->ipcp_msgs)) {
		IPC_PORT_UNLOCK(ipcp);
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
		error = ipc_port_right_insert(ipcp, task, ipcmsg->ipcmsg_header.ipchdr_right);
		if (error != 0)
			panic("%s: grating rights failed: %m", __func__,
			      error);
		IPC_PORT_UNLOCK(ipcp);
	}

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
ipc_port_right_drop(ipc_port_t port, ipc_port_right_t right)
{
	struct ipc_port *ipcp;
	int error;

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}
	IPC_PORTS_UNLOCK();

	error = ipc_port_right_remove(ipcp, current_task(), right);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		return (error);
	}
	IPC_PORT_UNLOCK(ipcp);
	return (0);
}

int
ipc_port_right_grant(struct task *task, ipc_port_t src, ipc_port_right_t right)
{
	struct ipc_port *ipcp;
	int error;

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(src);
	if (ipcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}
	IPC_PORTS_UNLOCK();

	if (!ipc_port_right_check(ipcp, current_task(), IPC_PORT_RIGHT_RECEIVE)) {
		IPC_PORT_UNLOCK(ipcp);
		return (ERROR_NO_RIGHT);
	}

	error = ipc_port_right_insert(ipcp, task, right);
	if (error != 0) {
		IPC_PORT_UNLOCK(ipcp);
		return (error);
	}
	IPC_PORT_UNLOCK(ipcp);
	return (0);
}

int
ipc_port_right_send(ipc_port_t dst, ipc_port_t src, ipc_port_right_t right)
{
	struct ipc_port *dstp, *srcp;
	struct ipc_port_right *ipcpr;
	int error;

	IPC_PORTS_LOCK();
	srcp = ipc_port_lookup(src);
	if (srcp == NULL) {
		IPC_PORTS_UNLOCK();
		return (ERROR_NOT_FOUND);
	}
	IPC_PORTS_UNLOCK();

	if (!ipc_port_right_check(srcp, current_task(), IPC_PORT_RIGHT_RECEIVE)) {
		IPC_PORT_UNLOCK(srcp);
		return (ERROR_NO_RIGHT);
	}

	dstp = ipc_port_lookup(dst);
	if (dstp == NULL) {
		IPC_PORT_UNLOCK(srcp);
		return (ERROR_NOT_FOUND);
	}

	/*
	 * For each task with a receive right on dst,
	 * grant it the requested right on src.
	 */
	BTREE_MIN(ipcpr, &dstp->ipcp_rights, ipcpr_node);
	while (ipcpr != NULL) {
		if ((ipcpr->ipcpr_right & IPC_PORT_RIGHT_RECEIVE) != 0) {
			error = ipc_port_right_insert(srcp, ipcpr->ipcpr_task, right);
			if (error != 0)
				printf("%s: ipc_port_right_insert failed: %m\n", __func__, error);
		}

		BTREE_NEXT(ipcpr, ipcpr_node);
	}
	IPC_PORT_UNLOCK(dstp);
	IPC_PORT_UNLOCK(srcp);

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

	if (!ipc_port_right_check(ipcp, task, IPC_PORT_RIGHT_RECEIVE)) {
		IPC_PORT_UNLOCK(ipcp);
		IPC_PORTS_UNLOCK();
		return (ERROR_NO_RIGHT);
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
		if (!ipc_port_right_check(ipcp, task, IPC_PORT_RIGHT_SEND)) {
			IPC_PORT_UNLOCK(ipcp);
			IPC_PORTS_UNLOCK();
			return (ERROR_NO_RIGHT);
		}
	}
	IPC_PORTS_UNLOCK();

	ipcmsg = malloc(sizeof *ipcmsg);
	ipcmsg->ipcmsg_header = *ipch;
	ipcmsg->ipcmsg_page = page;

	TAILQ_INSERT_TAIL(&ipcp->ipcp_msgs, ipcmsg, ipcmsg_link);
	cv_signal(ipcp->ipcp_cv);

	IPC_PORT_UNLOCK(ipcp);

	return (0);
}

int
ipc_port_wait(ipc_port_t port)
{
	struct ipc_port *ipcp;
	struct task *task;

	task = current_task();

	ASSERT(task != NULL, "Must have a running task.");

	IPC_PORTS_LOCK();
	ipcp = ipc_port_lookup(port);
	ASSERT(ipcp != NULL, "Must have a port to wait on.");
	IPC_PORTS_UNLOCK();
	if (!TAILQ_EMPTY(&ipcp->ipcp_msgs)) {
		/*
		 * XXX
		 * Should we do the right check first?
		 * Is there a security issue with letting
		 * other tasks work out whether a port has
		 * messages pending?  Probably.
		 */
		IPC_PORT_UNLOCK(ipcp);
		return (0);
	}

	if (!ipc_port_right_check(ipcp, task, IPC_PORT_RIGHT_RECEIVE)) {
		IPC_PORT_UNLOCK(ipcp);
		return (ERROR_NO_RIGHT);
	}

	/* XXX refcount.  */

	cv_wait(ipcp->ipcp_cv);

	return (0);
}

static struct ipc_port *
ipc_port_alloc(void)
{
	struct ipc_port *ipcp;
	struct task *task;
	int error;

	task = current_task();

	ipcp = pool_allocate(&ipc_port_pool);
	mutex_init(&ipcp->ipcp_mutex, "IPC Port", MUTEX_FLAG_DEFAULT);
	ipcp->ipcp_cv = cv_create(&ipcp->ipcp_mutex);
	ipcp->ipcp_port = IPC_PORT_UNKNOWN;
	ipcp->ipcp_flags = IPC_PORT_FLAG_NEW;
	TAILQ_INIT(&ipcp->ipcp_msgs);
	BTREE_NODE_INIT(&ipcp->ipcp_link);
	BTREE_ROOT_INIT(&ipcp->ipcp_rights);

	/*
	 * Insert a receive right.
	 */
	error = ipc_port_right_insert(ipcp, task, IPC_PORT_RIGHT_RECEIVE);
	if (error != 0)
		panic("%s: ipc_port_right_insert failed: %m", __func__,
		      error);


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
ipc_port_register(struct ipc_port *ipcp, ipc_port_t port, ipc_port_flags_t flags)
{
	struct ipc_port *old, *iter;

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

	ipcp->ipcp_flags &= ~IPC_PORT_FLAG_NEW;
	ipcp->ipcp_flags |= flags;

	return (0);
}

static bool
ipc_port_right_check(struct ipc_port *ipcp, struct task *task, ipc_port_right_t right)
{
	struct ipc_port_right *ipcpr;

	if ((right & IPC_PORT_RIGHT_SEND_ONCE) != 0)
		panic("%s: must check for a send right, not a reply right.", __func__);

	/*
	 * XXX Assert that right is exactly one right.
	 */

	ipcpr = ipc_port_right_lookup(ipcp, task);
	if (ipcpr == NULL)
		return (false);

	/*
	 * If the port right includes a receive right, allow anything.
	 */
	if ((ipcpr->ipcpr_right & IPC_PORT_RIGHT_RECEIVE) != 0)
		return (true);

	/*
	 * If there was no receive right, we must only be looking for a send right
	 * to succeed.
	 */
	if (right != IPC_PORT_RIGHT_SEND)
		return (false);

	/*
	 * If there is a send right, use it.
	 */
	if ((ipcpr->ipcpr_right & IPC_PORT_RIGHT_SEND) != 0)
		return (true);

	/*
	 * If there is a send-once right, consume it.
	 */
	if ((ipcpr->ipcpr_right & IPC_PORT_RIGHT_SEND_ONCE) != 0) {
		if (ipcpr->ipcpr_right == IPC_PORT_RIGHT_SEND_ONCE) {
			/*
			 * If there is only a send-once right, remove
			 * this right and free it.
			 * XXX TODO XXX
			 */
			ipcpr->ipcpr_right &= ~IPC_PORT_RIGHT_SEND_ONCE;
		} else {
			ipcpr->ipcpr_right &= ~IPC_PORT_RIGHT_SEND_ONCE;
		}
		return (true);
	}

	return (false);
}

static int
ipc_port_right_insert(struct ipc_port *ipcp, struct task *task, ipc_port_right_t right)
{
	struct ipc_port_right *ipcpr, *iter;

	/*
	 * If this is not a receive right (i.e. it is a send right) and the port
	 * is public, insert nothing.
	 */
	if ((right & IPC_PORT_RIGHT_RECEIVE) == 0) {
		if ((ipcp->ipcp_flags & IPC_PORT_FLAG_PUBLIC) != 0)
			return (0);
	}

	ipcpr = ipc_port_right_lookup(ipcp, task);
	if (ipcpr != NULL) {
		/*
		 * XXX
		 * This logic is not quite right, I guess.
		 */
		ipcpr->ipcpr_right |= right;
		return (0);
	}

	/*
	 * We've been asked to create an absent right.
	 */
	ipcpr = pool_allocate(&ipc_port_right_pool);
	ipcpr->ipcpr_task = task;
	ipcpr->ipcpr_right = right;
	BTREE_NODE_INIT(&ipcpr->ipcpr_node);

	BTREE_INSERT(ipcpr, iter, &ipcp->ipcp_rights, ipcpr_node,
		     (ipcpr->ipcpr_task < iter->ipcpr_task));
	STAILQ_INSERT_TAIL(&task->t_ipc.ipct_rights, ipcpr, ipcpr_link);

	return (0);
}

static struct ipc_port_right *
ipc_port_right_lookup(struct ipc_port *ipcp, struct task *task)
{
	struct ipc_port_right *ipcpr, *iter;

	BTREE_FIND(&ipcpr, iter, &ipcp->ipcp_rights, ipcpr_node,
		   (task < iter->ipcpr_task), (task == iter->ipcpr_task));
	return (ipcpr);
}

static int
ipc_port_right_remove(struct ipc_port *ipcp, struct task *task, ipc_port_right_t right)
{
	struct ipc_port_right *ipcpr;

	ipcpr = ipc_port_right_lookup(ipcp, task);
	if (ipcpr == NULL)
		return (ERROR_NO_RIGHT);

	/* XXX
	 * The logic here is wrong.
	 * XXX TODO XXX
	 * Decrement receive right count?
	 * Handle send-once dropping?
	 * Free!
	 */
	ipcpr->ipcpr_right &= ~right;

	return (0);
}

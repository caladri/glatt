#ifndef	_CORE_IPC_H_
#define	_CORE_IPC_H_

#include <core/mutex.h>
#include <core/queue.h>

typedef	uint64_t	ipc_port_t;
typedef	int64_t		ipc_msg_t;
typedef	uint64_t	ipc_size_t;

#define	IPC_PORT_RESERVED_START		(0)
#define	IPC_PORT_RESERVED_END		(100)
#define	IPC_PORT_UNRESERVED_START	(100)
#define	IPC_PORT_UNRESERVED_END		((ipc_port_t)1 << ((8 * sizeof (ipc_port_t)) - 1))

struct ipc_port {
	struct mutex ipcp_mutex;
	ipc_port_t ipcp_port;
	struct vdae_list *ipcp_vdae;
	TAILQ_HEAD(, struct ipc_message) ipcp_msgs;
	TAILQ_ENTRY(struct ipc_port) ipcp_link;
};

struct ipc_header {
	ipc_port_t ipchdr_src;
	ipc_port_t ipchdr_dst;
	ipc_msg_t ipchdr_type;
	ipc_size_t ipchdr_len;
};

struct ipc_data {
	int dummy;
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

void ipc_init(void);
void ipc_process(void);

int ipc_port_allocate(ipc_port_t *);
void ipc_port_free(ipc_port_t);
int ipc_port_receive(ipc_port_t, struct ipc_header *, struct ipc_data *);

#endif /* !_CORE_IPC_H_ */

#ifndef	_CORE_IPC_H_
#define	_CORE_IPC_H_

struct task;

typedef	uint64_t	ipc_port_t;
typedef	int64_t		ipc_msg_t;
typedef	uint64_t	ipc_size_t;

#define	IPC_PORT_UNKNOWN		(0)
#define	IPC_PORT_NS			(1)
#define	IPC_PORT_UNRESERVED_START	(2)

struct ipc_header {
	ipc_port_t ipchdr_src;
	ipc_port_t ipchdr_dst;
	ipc_msg_t ipchdr_msg;
};

#define	IPC_MSG_REPLY(msg)	(-(msg))

#define	IPC_HEADER_REPLY(ipchdrp)					\
	do {								\
		ipc_port_t _reply = (ipchdrp)->ipchdr_src;		\
		(ipchdrp)->ipchdr_src = (ipchdrp)->ipchdr_dst;		\
		(ipchdrp)->ipchdr_dst = _reply;				\
		(ipchdrp)->ipchdr_msg = -(ipchdrp)->ipchdr_msg;		\
	} while (0)

void ipc_init(void);
void ipc_process(void);
int ipc_send(struct ipc_header *, vaddr_t *);

int ipc_port_allocate(struct task *, ipc_port_t *);
int ipc_port_allocate_reserved(struct task *, ipc_port_t);
void ipc_port_free(ipc_port_t);
int ipc_port_receive(ipc_port_t, struct ipc_header *, vaddr_t *);
void ipc_port_wait(ipc_port_t);

#endif /* !_CORE_IPC_H_ */

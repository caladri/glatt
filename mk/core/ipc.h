#ifndef	_CORE_IPC_H_
#define	_CORE_IPC_H_

#include <vm/types.h>

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

void ipc_init(void);
void ipc_process(void);
int ipc_send(struct ipc_header *, vaddr_t *);

int ipc_port_allocate(ipc_port_t *);
int ipc_port_allocate_reserved(ipc_port_t *, ipc_port_t);
void ipc_port_free(ipc_port_t);
int ipc_port_receive(ipc_port_t, struct ipc_header *, vaddr_t *);
void ipc_port_wait(ipc_port_t);

#endif /* !_CORE_IPC_H_ */

#ifndef	_CORE_IPC_H_
#define	_CORE_IPC_H_

#include <core/mutex.h>
#include <core/queue.h>

typedef	uint64_t	ipc_port_t;
typedef	int64_t		ipc_msg_t;
typedef	uint64_t	ipc_size_t;

#define	IPC_PORT_UNKNOWN		(0)
#define	IPC_PORT_RESERVED_START		(1)
#define	IPC_PORT_RESERVED_END		(100)
#define	IPC_PORT_UNRESERVED_START	(100)
#define	IPC_PORT_UNRESERVED_END		((ipc_port_t)1 << ((8 * sizeof (ipc_port_t)) - 1))

struct ipc_header {
	ipc_port_t ipchdr_src;
	ipc_port_t ipchdr_dst;
	ipc_msg_t ipchdr_type;
	ipc_size_t ipchdr_len;
};

void ipc_init(void);
void ipc_process(void);
int ipc_send(struct ipc_header *);

int ipc_port_allocate(ipc_port_t *);
void ipc_port_free(ipc_port_t);
int ipc_port_receive(ipc_port_t, struct ipc_header *);
void ipc_port_wait(ipc_port_t);

#endif /* !_CORE_IPC_H_ */

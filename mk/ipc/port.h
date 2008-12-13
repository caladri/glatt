#ifndef	_IPC_PORT_H_
#define	_IPC_PORT_H_

struct ipc_data;
struct ipc_header;
struct task;

void ipc_port_init(void);

int ipc_port_allocate(struct task *, ipc_port_t *);
int ipc_port_allocate_reserved(struct task *, ipc_port_t);
int ipc_port_receive(ipc_port_t, struct ipc_header *, struct ipc_data **);
int ipc_port_send(struct ipc_header *, struct ipc_data *);
int ipc_port_wait(ipc_port_t);

#endif /* !_IPC_PORT_H_ */

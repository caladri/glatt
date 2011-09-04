#ifndef	_IPC_PORT_H_
#define	_IPC_PORT_H_

struct ipc_data;
struct ipc_header;
struct task;

typedef	unsigned	ipc_port_flags_t;

#define	IPC_PORT_FLAG_DEFAULT	(0x00000000)
#define	IPC_PORT_FLAG_NEW	(0x00000001)
#define	IPC_PORT_FLAG_PUBLIC	(0x00000002)

void ipc_port_init(void);

int ipc_port_allocate(struct task *, ipc_port_t *, ipc_port_flags_t) __non_null(1, 2) __check_result;
int ipc_port_allocate_reserved(struct task *, ipc_port_t, ipc_port_flags_t) __non_null(1) __check_result;
int ipc_port_receive(ipc_port_t, struct ipc_header *, struct ipc_data **) __non_null(2) __check_result;
int ipc_port_send(struct ipc_header *, struct ipc_data *) __non_null(1) __check_result;
int ipc_port_wait(ipc_port_t) __check_result;

#endif /* !_IPC_PORT_H_ */

#ifndef	_IPC_PORT_H_
#define	_IPC_PORT_H_

struct ipc_data;
struct ipc_header;
#ifdef MK
struct task;
struct vm_page;
#endif

typedef	unsigned	ipc_port_flags_t;

#define	IPC_PORT_FLAG_DEFAULT	(0x00000000)
#define	IPC_PORT_FLAG_NEW	(0x00000001)
#define	IPC_PORT_FLAG_PUBLIC	(0x00000002)

#ifdef MK
void ipc_port_init(void);

int ipc_port_allocate(struct task *, ipc_port_t *, ipc_port_flags_t) __non_null(1, 2) __check_result;
int ipc_port_allocate_reserved(struct task *, ipc_port_t, ipc_port_flags_t) __non_null(1) __check_result;
#else
int ipc_port_allocate(ipc_port_t *, ipc_port_flags_t);
#endif
int ipc_port_receive(ipc_port_t, struct ipc_header *, void **) __non_null(2) __check_result;
int ipc_port_send(struct ipc_header *, void *) __non_null(1) __check_result;
int ipc_port_send_data(struct ipc_header *, const void *, size_t) __non_null(1) __check_result;
#ifdef MK
int ipc_port_send_page(struct ipc_header *, struct vm_page *) __non_null(1) __check_result;
int ipc_port_send_right(ipc_port_t, ipc_port_t, ipc_port_right_t) __check_result;
#endif
int ipc_port_wait(ipc_port_t) __check_result;

#endif /* !_IPC_PORT_H_ */

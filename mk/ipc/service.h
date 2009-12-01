#ifndef	_IPC_SERVICE_H_
#define	_IPC_SERVICE_H_

struct ipc_data;
struct ipc_header;

typedef	int (ipc_service_t)(void *, struct ipc_header *, struct ipc_data *);

int ipc_service(const char *, ipc_port_t, ipc_service_t *, void *) __non_null(1, 3);

#endif /* !_IPC_SERVICE_H_ */

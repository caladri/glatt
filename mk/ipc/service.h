#ifndef	_IPC_SERVICE_H_
#define	_IPC_SERVICE_H_

typedef	int (ipc_service_t)(void *, struct ipc_header *);

int ipc_service(const char *, ipc_port_t, ipc_service_t *, void *);

#endif /* !_IPC_SERVICE_H_ */

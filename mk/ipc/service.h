#ifndef	_IPC_SERVICE_H_
#define	_IPC_SERVICE_H_

struct ipc_data;
struct ipc_header;

typedef	int (ipc_service_t)(void *, struct ipc_header *, struct ipc_data *) __non_null(2);

int ipc_service(const char *, ipc_port_t, ipc_port_flags_t, ipc_service_t *, void *) __non_null(1, 4) __check_result;

#endif /* !_IPC_SERVICE_H_ */

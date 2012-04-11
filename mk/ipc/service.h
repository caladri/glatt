#ifndef	_IPC_SERVICE_H_
#define	_IPC_SERVICE_H_

struct ipc_header;
struct ipc_service_context;

typedef	int (ipc_service_t)(void *, struct ipc_header *, void *) __non_null(2) __check_result;

int ipc_service(const char *, bool, ipc_port_t, ipc_port_flags_t, const struct ipc_service_context **, ipc_service_t *, void *) __non_null(1, 6) __check_result;
ipc_port_t ipc_service_port(const struct ipc_service_context *);

#endif /* !_IPC_SERVICE_H_ */

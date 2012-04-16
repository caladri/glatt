#ifndef	_IPC_SERVICE_H_
#define	_IPC_SERVICE_H_

struct ipc_header;
struct ipc_service_context;
struct ipc_token;

typedef	int (ipc_service_t)(void *, struct ipc_header *, void *) __non_null(2) __check_result;

int ipc_service(const char *, ipc_port_t, ipc_port_flags_t, struct ipc_service_context **, ipc_service_t *, void *) __non_null(1, 5) __check_result;
ipc_port_t ipc_service_port(struct ipc_service_context *) __non_null(1) __check_result;
struct ipc_token *ipc_service_token(struct ipc_service_context *) __non_null(1) __check_result;
void ipc_service_start(struct ipc_service_context *) __non_null(1);

#endif /* !_IPC_SERVICE_H_ */

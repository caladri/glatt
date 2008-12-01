#ifndef	_NS_SERVICE_DIRECTORY_H_
#define	_NS_SERVICE_DIRECTORY_H_

void service_directory_init(void);

int service_directory_enter(const char *, ipc_port_t);
int service_directory_lookup(const char *, ipc_port_t *);

#endif /* !_NS_SERVICE_DIRECTORY_H_ */

#ifndef	_NS_NS_H_
#define	_NS_NS_H_

	/* Constants.  */

#define	NS_SERVICE_NAME_LENGTH	(24)

	/* Message numbers and structures.  */

#define	NS_MESSAGE_LOOKUP	(0x00000001)

struct ns_lookup_request {
	char service_name[NS_SERVICE_NAME_LENGTH];
};

struct ns_lookup_response {
	ipc_port_t port;
};

struct ns_lookup_error {
	int error;
};

#define	NS_MESSAGE_REGISTER	(0x00000002)

struct ns_register_request {
	char service_name[NS_SERVICE_NAME_LENGTH];
	ipc_port_t port;
};

struct ns_register_error {
	int error;
};

#if !defined(MK)
ipc_port_t ns_lookup(const char *);
int ns_register(const char *, ipc_port_t);
#endif

#endif /* !_NS_NS_H_ */

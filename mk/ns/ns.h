#ifndef	_NS_NS_H_
#define	_NS_NS_H_

	/* Constants.  */

#define	NS_SERVICE_NAME_LENGTH	(24)

	/* Common types.  */

	/* Message types.  */

#define	NS_MINIMAL_MESSAGE_FIELDS					\
	int error

#define	NS_SERVICE_NAME_MESSAGE_FIELDS					\
	NS_MINIMAL_MESSAGE_FIELDS;					\
	char service_name[NS_SERVICE_NAME_LENGTH]

#define	NS_PORT_MESSAGE_FIELDS						\
	NS_SERVICE_NAME_MESSAGE_FIELDS;					\
	ipc_port_t port

#define	NS_MESSAGE(msg, reqtype, resptype)				\
	struct ns_ ## msg ## _request {					\
		_CONCAT(reqtype, _MESSAGE_FIELDS);			\
	};								\
									\
	struct ns_ ## msg ## _response {				\
		_CONCAT(resptype, _MESSAGE_FIELDS);			\
	}

	/* Message numbers and structures.  */

#define	NS_MESSAGE_LOOKUP	(0x00000001)

NS_MESSAGE(lookup, NS_SERVICE_NAME, NS_PORT);

#define	NS_MESSAGE_REGISTER	(0x00000002)

NS_MESSAGE(register, NS_PORT, NS_PORT);

#if !defined(MK)
ipc_port_t ns_lookup(const char *);
int ns_register(const char *, ipc_port_t);
#endif

#endif /* !_NS_NS_H_ */

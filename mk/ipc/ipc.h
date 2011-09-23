#ifndef	_IPC_IPC_H_
#define	_IPC_IPC_H_

#include <core/queue.h>

typedef	uint32_t	ipc_port_t;
typedef	int32_t		ipc_msg_t;
typedef	uint32_t	ipc_port_right_t;
typedef	uint64_t	ipc_cookie_t;
typedef	uint16_t	ipc_size_t;

#define	IPC_PORT_UNKNOWN		(0)
#define	IPC_PORT_NS			(1)
#define	IPC_PORT_UNRESERVED_START	(2)

#define	IPC_PORT_RIGHT_NONE		(0x00000000)
#define	IPC_PORT_RIGHT_SEND		(0x00000001)
#define	IPC_PORT_RIGHT_SEND_ONCE	(0x00000002)
#define	IPC_PORT_RIGHT_RECEIVE		(0x00000004)

/*
 * The minimum fields a message contains.
 */
struct ipc_header {
	ipc_port_t ipchdr_src;
	ipc_port_t ipchdr_dst;
	ipc_port_right_t ipchdr_right;	/* Right to give dst on src.  */
	ipc_msg_t ipchdr_msg;		/* Opaque to IPC code.  */
	ipc_cookie_t ipchdr_cookie;	/* Opaque to IPC code.  */
	ipc_size_t ipchdr_recsize;	/* Opaque to IPC code.  */
	ipc_size_t ipchdr_reccnt;	/* Opaque to IPC code.  */
};

#define	IPC_MSG_REPLY(msg)	(-(msg))

#define	IPC_HEADER_REPLY(ipchdrp)					\
	((struct ipc_header) {						\
		.ipchdr_src = (ipchdrp)->ipchdr_dst,			\
		.ipchdr_dst = (ipchdrp)->ipchdr_src,			\
		.ipchdr_msg = IPC_MSG_REPLY((ipchdrp)->ipchdr_msg),	\
		.ipchdr_right = IPC_PORT_RIGHT_NONE,			\
	 	.ipchdr_cookie = (ipchdrp)->ipchdr_cookie,		\
	 	.ipchdr_recsize = 0,					\
	 	.ipchdr_reccnt = 0,					\
	})

#if !defined(MK)
#include <ipc/port.h>
#endif

#endif /* !_IPC_IPC_H_ */

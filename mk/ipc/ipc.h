#ifndef	_IPC_IPC_H_
#define	_IPC_IPC_H_

#if defined(MK)
#include <core/queue.h>
#endif

typedef	uint32_t	ipc_port_t;
typedef	uint16_t	ipc_port_right_t;
typedef	uint16_t	ipc_msg_t;
typedef	uint64_t	ipc_cookie_t;
typedef	uint64_t	ipc_parameter_t;

#define	IPC_PORT_UNKNOWN		(0)
#define	IPC_PORT_NS			(1)
#define	IPC_PORT_CONSOLE		(2)
#define	IPC_PORT_UNRESERVED_START	(3)

#define	IPC_PORT_RIGHT_NONE		(0x0000)
#define	IPC_PORT_RIGHT_SEND		(0x0001)
#define	IPC_PORT_RIGHT_SEND_ONCE	(0x0002)
#define	IPC_PORT_RIGHT_RECEIVE		(0x0004)

/*
 * The minimum fields a message contains.
 */
struct ipc_header {
	ipc_port_t ipchdr_src;
	ipc_port_t ipchdr_dst;
	ipc_port_right_t ipchdr_right;	/* Right to give dst on src.  */
	ipc_msg_t ipchdr_msg;		/* Opaque to IPC code, except IPC_MSG_NONE.  */
	ipc_cookie_t ipchdr_cookie;	/* Opaque to IPC code.  */
	ipc_parameter_t ipchdr_param;	/* Opaque to IPC code.  */
};

/*
 * Message field encoding.
 *
 * XXX Is this still a desirable idea?
 */
#define	IPC_MSG_NONE		(0)	/* Requires no right to send, may not have data.  */

/*
 * These are conventional but not semantic to the IPC code.
 */
#define	IPC_MSG_FLAG_MASK	(0xf000)
#define	IPC_MSG_MASK		(0x0fff)

#define	IPC_MSG_FLAG_REQUEST	(0x0000)
#define	IPC_MSG_FLAG_REPLY	(0x1000)
#define	IPC_MSG_FLAG_ERROR	(0x2000)

#define	IPC_MSG_REQUEST(msg)	(IPC_MSG_FLAG_REQUEST | ((msg) & IPC_MSG_MASK))
#define	IPC_MSG_REPLY(msg)	(IPC_MSG_FLAG_REPLY | ((msg) & IPC_MSG_MASK))
#define	IPC_MSG_ERROR(msg)	(IPC_MSG_FLAG_ERROR | ((msg) & IPC_MSG_MASK))

#define	IPC_HEADER_REPLY(ipchdrp)					\
	((struct ipc_header) {						\
		.ipchdr_src = (ipchdrp)->ipchdr_dst,			\
		.ipchdr_dst = (ipchdrp)->ipchdr_src,			\
		.ipchdr_msg = IPC_MSG_REPLY((ipchdrp)->ipchdr_msg),	\
		.ipchdr_right = IPC_PORT_RIGHT_NONE,			\
	 	.ipchdr_cookie = (ipchdrp)->ipchdr_cookie,		\
	 	.ipchdr_param = 0,					\
	})

#define	IPC_HEADER_ERROR(ipchdrp, error)				\
	((struct ipc_header) {						\
		.ipchdr_src = (ipchdrp)->ipchdr_dst,			\
		.ipchdr_dst = (ipchdrp)->ipchdr_src,			\
		.ipchdr_msg = IPC_MSG_ERROR((ipchdrp)->ipchdr_msg),	\
		.ipchdr_right = IPC_PORT_RIGHT_NONE,			\
	 	.ipchdr_cookie = (ipchdrp)->ipchdr_cookie,		\
	 	.ipchdr_param = (error),				\
	})

#if !defined(MK)
#include <ipc/port.h>
#endif

#endif /* !_IPC_IPC_H_ */

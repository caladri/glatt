#ifndef	_IPC_IPC_H_
#define	_IPC_IPC_H_

typedef	uint64_t	ipc_port_t;
typedef	int64_t		ipc_msg_t;
typedef	uint64_t	ipc_size_t;
typedef	uint32_t	ipc_port_right_t;

#define	IPC_PORT_UNKNOWN		(0)
#define	IPC_PORT_NS			(1)
#define	IPC_PORT_UNRESERVED_START	(2)

#define	IPC_PORT_RIGHT_NONE		(0x00000000)
#define	IPC_PORT_RIGHT_SEND		(0x00000001)
#define	IPC_PORT_RIGHT_SEND_ONCE	(0x00000002)
#define	IPC_PORT_RIGHT_RECEIVE	(0x00000004)

struct ipc_header {
	ipc_port_t ipchdr_src;
	ipc_port_t ipchdr_dst;
	ipc_port_right_t ipchdr_right;
	ipc_msg_t ipchdr_msg;
};

#define	IPC_MSG_REPLY(msg)	(-(msg))

#define	IPC_HEADER_REPLY(ipchdrp)					\
	do {								\
		ipc_port_t _reply = (ipchdrp)->ipchdr_src;		\
		(ipchdrp)->ipchdr_src = (ipchdrp)->ipchdr_dst;		\
		(ipchdrp)->ipchdr_dst = _reply;				\
		(ipchdrp)->ipchdr_right = IPC_PORT_RIGHT_NONE;		\
		(ipchdrp)->ipchdr_msg = -(ipchdrp)->ipchdr_msg;		\
	} while (0)

#endif /* !_IPC_IPC_H_ */

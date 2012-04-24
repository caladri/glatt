#ifndef	IPC_DISPATCH_H
#define	IPC_DISPATCH_H

struct ipc_dispatch_handler;

struct ipc_dispatch {
	ipc_port_t id_port;
	unsigned id_cookie_next;
	struct ipc_dispatch_handler *id_handlers;
	struct ipc_dispatch_handler *id_default;
};

typedef	void ipc_dispatch_callback_t(const struct ipc_dispatch *,
				     const struct ipc_dispatch_handler *,
				     const struct ipc_header *, void *);

struct ipc_dispatch_handler {
	unsigned idh_cookie;
	void *idh_softc;
	ipc_dispatch_callback_t *idh_callback;
	struct ipc_dispatch_handler *idh_next;
};

struct ipc_dispatch *ipc_dispatch_allocate(ipc_port_t, ipc_port_flags_t);
void ipc_dispatch_free(struct ipc_dispatch *);
void ipc_dispatch(const struct ipc_dispatch *);
bool ipc_dispatch_poll(const struct ipc_dispatch *);
const struct ipc_dispatch_handler *
	ipc_dispatch_register(struct ipc_dispatch *,
			      ipc_dispatch_callback_t *, void *);
const struct ipc_dispatch_handler *
	ipc_dispatch_register_default(struct ipc_dispatch *,
				      ipc_dispatch_callback_t *, void *);
int ipc_dispatch_send(const struct ipc_dispatch *,
		      const struct ipc_dispatch_handler *,
		      ipc_port_t, ipc_msg_t, ipc_port_right_t,
		      const void *, size_t);
int ipc_dispatch_send_reply(const struct ipc_dispatch *,
			    const struct ipc_header *, ipc_port_right_t,
			    const void *, size_t);
void ipc_dispatch_wait(const struct ipc_dispatch *);

#endif /* !IPC_DISPATCH_H */

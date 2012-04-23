#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/ipc_dispatch.h>

static void ipc_dispatch_message(const struct ipc_dispatch *, const struct ipc_header *, void *);

struct ipc_dispatch *
ipc_dispatch_allocate(ipc_port_t port, ipc_port_flags_t flags)
{
	struct ipc_dispatch *id;
	int error;

	id = malloc(sizeof *id);
	if (id == NULL)
		fatal("could not allocate memory for dispatch", ERROR_EXHAUSTED);

	if (port == IPC_PORT_UNKNOWN) {
		error = ipc_port_allocate(&port, flags);
		if (error != 0)
			fatal("could not allocate port", error);
	}

	id->id_port = port;
	id->id_cookie_next = 0;
	id->id_handlers = NULL;
	id->id_default = NULL;

	return (id);
}

void
ipc_dispatch_free(struct ipc_dispatch *id)
{
	struct ipc_dispatch_handler *idh;

	while ((idh = id->id_handlers) != NULL) {
		id->id_handlers = idh->idh_next;
		free(idh);
	}

	if (id->id_default != NULL)
		free(id->id_default);

	/* XXX free port.  */

	free(id);
}

void
ipc_dispatch(const struct ipc_dispatch *id)
{
	struct ipc_header ipch;
	void *page;
	int error;

	if (id->id_handlers == NULL && id->id_default == NULL)
		fatal("no handlers registered", ERROR_UNEXPECTED);

	for (;;) {
		error = ipc_port_wait(id->id_port);
		if (error != 0)
			fatal("ipc_port_wait failed", error);

		error = ipc_port_receive(id->id_port, &ipch, &page);
		if (error != 0)
			fatal("ipc_port_receive failed", error);

		ipc_dispatch_message(id, &ipch, page);
	}
}

bool
ipc_dispatch_poll(const struct ipc_dispatch *id)
{
	struct ipc_header ipch;
	void *page;
	int error;

	if (id->id_handlers == NULL && id->id_default == NULL)
		fatal("no handlers registered", ERROR_UNEXPECTED);

	error = ipc_port_receive(id->id_port, &ipch, &page);
	if (error != 0)
		return (false);

	ipc_dispatch_message(id, &ipch, page);

	return (false);
}

const struct ipc_dispatch_handler *
ipc_dispatch_register(struct ipc_dispatch *id, ipc_dispatch_callback_t *cb, void *softc)
{
	struct ipc_dispatch_handler *idh;

	idh = malloc(sizeof *idh);
	if (idh == NULL)
		fatal("could not allocate memory for handler", ERROR_EXHAUSTED);

	idh->idh_cookie = id->id_cookie_next++;
	idh->idh_softc = softc;
	idh->idh_callback = cb;
	idh->idh_next = id->id_handlers;
	id->id_handlers = idh;

	return (idh);
}

const struct ipc_dispatch_handler *
ipc_dispatch_register_default(struct ipc_dispatch *id, ipc_dispatch_callback_t *cb, void *softc)
{
	struct ipc_dispatch_handler *idh;

	if (id->id_default != NULL)
		fatal("attempt to register two default handlers", ERROR_UNEXPECTED);

	idh = malloc(sizeof *idh);
	if (idh == NULL)
		fatal("could not allocate memory for handler", ERROR_EXHAUSTED);

	idh->idh_cookie = 0;
	idh->idh_softc = softc;
	idh->idh_callback = cb;
	idh->idh_next = NULL;
	id->id_default = idh;

	return (idh);
}

int
ipc_dispatch_send(const struct ipc_dispatch *id, const struct ipc_dispatch_handler *idh, ipc_port_t dst, ipc_msg_t msg, ipc_port_right_t right, const void *data, size_t datalen)
{
	struct ipc_header ipch;
	void *page;
	int error;

	if (data == NULL || datalen == 0) {
		page = NULL;
	} else {
		if (datalen > PAGE_SIZE)
			fatal("data too big for page", ERROR_NOT_IMPLEMENTED);
		error = vm_page_get(&page);
		if (error != 0)
			return (error);
		memcpy(page, data, datalen);
	}

	ipch.ipchdr_src = id->id_port;
	ipch.ipchdr_dst = dst;
	ipch.ipchdr_right = right;
	ipch.ipchdr_msg = msg;
	if (idh == NULL)
		ipch.ipchdr_cookie = 0;
	else
		ipch.ipchdr_cookie = idh->idh_cookie;

	error = ipc_port_send(&ipch, page);
	if (error != 0) {
		/*
		 * XXX
		 * Do we need to free page?
		 */
		return (error);
	}

	return (0);
}

int
ipc_dispatch_send_reply(const struct ipc_dispatch *id, const struct ipc_header *reqh, ipc_port_right_t right, const void *data, size_t datalen)
{
	struct ipc_header ipch;
	void *page;
	int error;

	if (data == NULL || datalen == 0) {
		page = NULL;
	} else {
		if (datalen > PAGE_SIZE)
			fatal("data too big for page", ERROR_NOT_IMPLEMENTED);
		error = vm_page_get(&page);
		if (error != 0)
			return (error);
		memcpy(page, data, datalen);
	}

	ipch = IPC_HEADER_REPLY(reqh);
	ipch.ipchdr_src = id->id_port;
	ipch.ipchdr_right = right;

	error = ipc_port_send(&ipch, page);
	if (error != 0) {
		/*
		 * XXX
		 * Do we need to free page?
		 */
		return (error);
	}

	return (0);

}

void
ipc_dispatch_wait(const struct ipc_dispatch *id)
{
	int error;

	error = ipc_port_wait(id->id_port);
	if (error != 0)
		fatal("ipc_port_wait failed", error);
}

static void
ipc_dispatch_message(const struct ipc_dispatch *id, const struct ipc_header *ipch, void *page)
{
	struct ipc_dispatch_handler *idh;

	for (idh = id->id_handlers; idh != NULL; idh = idh->idh_next) {
		if (idh->idh_cookie != ipch->ipchdr_cookie)
			continue;
		idh->idh_callback(id, idh, ipch, page);
		return;
	}

	if (id->id_default != NULL) {
		id->id_default->idh_callback(id, id->id_default, ipch, page);
		return;
	}

	ipc_message_drop(ipch, page);
}

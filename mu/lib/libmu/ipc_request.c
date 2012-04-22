#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ns/ns.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/ipc_request.h>

/*
 * An ideal synchronous IPC API would ask the user:
 * 	1) What port are you sending from? [optional]
 * 	2) What port are you sending to?
 * 	3) Are you sending any data?
 * 		a) Is it a page to be flipped?
 * 		b) Is it data we should copy to a page?
 * 	4) What kind of message are you sending?
 * 	5) Do you anticipate any data in the reply case?
 *
 * It would not handle more complex cases such as when the
 * error case has more than an errno.  It has no need to
 * handle the case where there is no reply.  It has no need
 * to handle the case where the right sent is anything but
 * SEND_ONCE -- if it's SEND, you'll need a handler loop
 * anyway and aren't expecting a mere response.
 */

int
ipc_request(const struct ipc_request_message *req, struct ipc_response_message *resp)
{
	struct ipc_header ipch;
	ipc_port_t req_port;
	int error, error2;
	void *page;

	if (req->data != NULL && req->datalen != 0) {
		if (req->datalen > PAGE_SIZE)
			return (ERROR_INVALID);
		error = vm_page_get(&page);
		if (error != 0)
			return (error);
		memcpy(page, req->data, req->datalen);
	} else {
		page = req->page;
	}

	if (req->src == IPC_PORT_UNKNOWN) {
		error = ipc_port_allocate(&req_port, IPC_PORT_FLAG_DEFAULT);
		if (error != 0) {
			if (page != NULL) {
				error2 = vm_page_free(page);
				if (error2 != 0)
					fatal("vm_page_free failed", error2);
			}
			return (error);
		}
	} else {
		req_port = req->src;
	}

	ipch.ipchdr_src = req_port;
	ipch.ipchdr_dst = req->dst;
	ipch.ipchdr_right = IPC_PORT_RIGHT_SEND_ONCE;
	ipch.ipchdr_msg = req->msg;
	ipch.ipchdr_cookie = (ipc_cookie_t)(uintptr_t)req;
	ipch.ipchdr_param = req->param;
	if (req->data != NULL)
		ipch.ipchdr_recsize = req->datalen;
	else if (req->page != NULL)
		ipch.ipchdr_recsize = req->recsize;
	else
		ipch.ipchdr_recsize = 0;

	error = ipc_port_send(&ipch, page);
	if (error != 0) {
		/*
		 * XXX
		 * Free req_port.
		 */
		return (error);
	}

	for (;;) {
		error = ipc_port_wait(req_port);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			/*
			 * XXX
			 * Free req_port.
			 */
			return (error);
		}

		if (resp->data)
			error = ipc_port_receive(req_port, &ipch, &page);
		else
			error = ipc_port_receive(req_port, &ipch, NULL);
		if (error != 0) {
			if (error == ERROR_AGAIN)
				continue;
			/*
			 * XXX
			 * Free req_port.
			 */
			return (error);
		}

		/*
		 * Some other port sending to us, go to
		 * the next message.
		 */
		if (ipch.ipchdr_src != req->dst) {
			if (resp->data && page != NULL) {
				error = vm_page_free(page);
				if (error != 0)
					fatal("vm_page_free failed", error);
			}
			continue;
		}

		/*
		 * From the right port, but wrong message.
		 * That's an error.
		 */
		if ((ipch.ipchdr_msg & IPC_MSG_MASK) !=
		    (req->msg & IPC_MSG_MASK) ||
		    (ipch.ipchdr_cookie != (ipc_cookie_t)(uintptr_t)req)) {
			if (resp->data && page != NULL) {
				error = vm_page_free(page);
				if (error != 0)
					fatal("vm_page_free failed", error);
			}
			return (ERROR_UNEXPECTED);
		}

		switch (ipch.ipchdr_msg & IPC_MSG_FLAG_MASK) {
		case IPC_MSG_FLAG_REPLY:
			resp->param = ipch.ipchdr_param;
			if (resp->data)
				resp->page = page;
			return (0);
		case IPC_MSG_FLAG_ERROR:
			if (resp->data && page != NULL) {
				/*
				 * No data may be sent with
				 * error messages right now.
				 */
				error = vm_page_free(page);
				if (error != 0)
					fatal("vm_page_free failed", error);
				return (ERROR_UNEXPECTED);
			}
			/*
			 * If the remote side indicated error
			 * but not a specific error, just give
			 * ERROR_UNEXPECTED, which is about as
			 * catch-all as catch can.
			 */
			if (ipch.ipchdr_param == 0)
				resp->error = ERROR_UNEXPECTED;
			else
				resp->error = ipch.ipchdr_param;
			return (0);
		default:
			if (resp->data && page != NULL) {
				error = vm_page_free(page);
				if (error != 0)
					fatal("vm_page_free failed", error);
			}
			return (ERROR_UNEXPECTED);
		}
	}
}

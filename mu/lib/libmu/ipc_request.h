#ifndef	IPC_REQUEST_H
#define	IPC_REQUEST_H

struct ipc_request_message {
	ipc_port_t src;
	ipc_port_t dst;
	ipc_msg_t msg;
	ipc_parameter_t param;

	/*
	 * Messages can send a copy of data.
	 */
	const void *data;
	ipc_size_t datalen;

	/*
	 * Or they can flip a page.
	 */
	void *page;
	ipc_size_t recsize; /* XXX This should go away soon! */
};

struct ipc_response_message {
	/*
	 * Is the caller expecting data in response?
	 */
	bool data;

	/*
	 * Success case.
	 */
	ipc_parameter_t param;
	void *page;

	/*
	 * Error case.
	 */
	int error;
};

int ipc_request(const struct ipc_request_message *, struct ipc_response_message *) __non_null(1, 2) __check_result;

#endif /* !IPC_REQUEST_H */

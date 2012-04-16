#ifndef	_IPC_TOKEN_H_
#define	_IPC_TOKEN_H_

/*
 * XXX
 * Tokens are an attempt at managing IPC rights in a straightforward and
 * secure way.  The API is a bit fat at present and could surely be smaller.
 * Its interactions with the port layer and the task layer are lousy.  This
 * is just an attempt to better understand the problem space.  They are sure
 * kind of ungainly.  And being able to ipc_token_allocate() anywhere is not
 * exactly very secure, but it's at least easy to spot trouble areas.
 *
 * The big functional goal is embodied in the IPC service code.  We allocate
 * a port and then we need to grant a receive right to the task that will be
 * reading from the port, and we also may need (i.e. in the case of non-public
 * services) to send that right via an IPC message to something else without
 * temporarily inserting a receive right into a task that has no business
 * having one for that port.
 */

struct ipc_token;

void ipc_token_init(void);
int ipc_token_allocate(struct ipc_token **, ipc_port_t, ipc_port_right_t) __check_result;
int ipc_token_allocate_child(const struct ipc_token *, struct ipc_token **, ipc_port_right_t) __check_result;
int ipc_token_consume(struct ipc_token *, ipc_port_right_t) __check_result;
void ipc_token_free(struct ipc_token *);
ipc_port_t ipc_token_port(const struct ipc_token *) __check_result;
int ipc_token_restrict(struct ipc_token *, ipc_port_right_t) __check_result;

#endif /* !_IPC_TOKEN_H_ */

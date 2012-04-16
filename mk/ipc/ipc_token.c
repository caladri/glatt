#include <core/types.h>
#include <core/error.h>
#include <core/mutex.h>
#include <core/pool.h>
#include <core/task.h>
#include <ipc/ipc.h>
#include <ipc/token.h>

struct ipc_token {
	ipc_port_t it_port;
	ipc_port_right_t it_right;
};

static struct pool ipc_token_pool;

static bool ipc_token_has_right(const struct ipc_token *, ipc_port_right_t);

void
ipc_token_init(void)
{
	int error;

	error = pool_create(&ipc_token_pool, "IPC TOKEN",
			    sizeof (struct ipc_token), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}

int
ipc_token_allocate(struct ipc_token **tokenp, ipc_port_t port, ipc_port_right_t right)
{
	struct ipc_token *token;

	token = pool_allocate(&ipc_token_pool);
	token->it_port = port;
	token->it_right = right;

	*tokenp = token;

	/*
	 * XXX
	 * Should we refcount ports?
	 */

	return (0);
}

int
ipc_token_allocate_child(const struct ipc_token *parent, struct ipc_token **tokenp, ipc_port_right_t right)
{
	if (ipc_token_has_right(parent, right))
		return (ipc_token_allocate(tokenp, parent->it_port, right));

	return (ERROR_NO_RIGHT);
}

int
ipc_token_consume(struct ipc_token *token, ipc_port_right_t right)
{
	if (ipc_token_has_right(token, right)) {
		ipc_token_free(token);
		return (0);
	}

	ipc_token_free(token);
	return (ERROR_NO_RIGHT);
}

void
ipc_token_free(struct ipc_token *token)
{
	pool_free(token);
}

ipc_port_t
ipc_token_port(const struct ipc_token *token)
{
	return (token->it_port);
}

int
ipc_token_restrict(struct ipc_token *token, ipc_port_right_t right)
{
	if (ipc_token_has_right(token, right)) {
		token->it_right = right;
		return (0);
	}

	return (ERROR_NO_RIGHT);
}

static bool
ipc_token_has_right(const struct ipc_token *token, ipc_port_right_t right)
{
	/*
	 * If the token token includes a receive right, allow anything.
	 */
	if ((token->it_right & IPC_PORT_RIGHT_RECEIVE) != 0)
		return (true);

	/*
	 * If the token token includes this right, allow it.
	 */
	if ((token->it_right & right) == right)
		return (true);

	/*
	 * If the requested right is a send-once right and we have a send right, allow it.
	 */
	if ((token->it_right & IPC_PORT_RIGHT_SEND) != 0 && right == IPC_PORT_RIGHT_SEND_ONCE)
		return (true);

	return (false);
}

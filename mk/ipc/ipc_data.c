#include <core/types.h>
#include <core/error.h>
#include <core/malloc.h>
#include <core/pool.h>
#include <core/string.h>
#include <ipc/data.h>
#include <ipc/ipc.h>
#include <ipc/port.h>

static struct pool ipc_data_pool;

void
ipc_data_init(void)
{
	int error;

	error = pool_create(&ipc_data_pool, "IPC DATA",
			    sizeof (struct ipc_data), POOL_VIRTUAL);
	if (error != 0)
		panic("%s: pool_create failed: %m", __func__, error);
}

int
ipc_data_copyin(struct ipc_data *srcp, struct ipc_data **dstp)
{
	struct ipc_data src, *ipcd;
	int error;

	ASSERT(dstp != NULL, "Destination pointer must not be NULL.");

	if (srcp == NULL) {
		return (0);
	}

	/* XXX copyin */
	src = *srcp;

	if (src.ipcd_type == IPC_DATA_TYPE_DEAD ||
	    src.ipcd_len == 0) {
		return (ipc_data_copyin(src.ipcd_next, dstp));
	}

	ipcd = pool_allocate(&ipc_data_pool);
	ipcd->ipcd_type = IPC_DATA_TYPE_DEAD;
	ipcd->ipcd_addr = NULL;
	ipcd->ipcd_len = 0;
	ipcd->ipcd_next = NULL;

	switch (src.ipcd_type) {
	case IPC_DATA_TYPE_SMALL:
		ipcd->ipcd_type = IPC_DATA_TYPE_SMALL;
		if (ipcd->ipcd_len > pool_max_alloc) {
			pool_free(ipcd);
			return (ERROR_INVALID);
		}
		ipcd->ipcd_addr = malloc(src.ipcd_len);
		ipcd->ipcd_len = src.ipcd_len;
		/* XXX copyin */
		memcpy(ipcd->ipcd_addr, src.ipcd_addr, ipcd->ipcd_len);
		break;
	default:
		pool_free(ipcd);
		return (ERROR_INVALID);
	}

	error = ipc_data_copyin(src.ipcd_next, &ipcd->ipcd_next);
	if (error != 0) {
		ipc_data_free(ipcd);
		return (error);
	}

	*dstp = ipcd;

	return (0);
}

void
ipc_data_free(struct ipc_data *ipcd)
{
	struct ipc_data *next;

	if (ipcd == NULL) {
		return;
	}
	
	switch (ipcd->ipcd_type) {
	case IPC_DATA_TYPE_DEAD:
		break;
	case IPC_DATA_TYPE_SMALL:
		if (ipcd->ipcd_addr != NULL) {
			free(ipcd->ipcd_addr);
		}

		ipcd->ipcd_addr = NULL;
		ipcd->ipcd_len = 0;
		break;
	default:
		panic("%s: unsupported ipc_data type %x", __func__,
		      ipcd->ipcd_type);
	}

	next = ipcd->ipcd_next;

	pool_free(ipcd);

	ipc_data_free(next);
}

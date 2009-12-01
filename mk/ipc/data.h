#ifndef	_IPC_DATA_H_
#define	_IPC_DATA_H_

typedef	uint8_t	ipc_data_type_t;

#define	IPC_DATA_TYPE_DEAD	(0x00)
#define	IPC_DATA_TYPE_SMALL	(0x01)

struct ipc_data {
	ipc_data_type_t ipcd_type;
	void *ipcd_addr;
	size_t ipcd_len;
	struct ipc_data *ipcd_next;
};

void ipc_data_init(void);

int ipc_data_copyin(struct ipc_data *, struct ipc_data **) __non_null(1, 2);
void ipc_data_free(struct ipc_data *) __non_null(1);

#endif /* !_IPC_DATA_H_ */

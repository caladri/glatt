#ifndef	_CORE_EXEC_H_
#define	_CORE_EXEC_H_

#include <fs/fs.h>
#include <fs/fs_ops.h>
#include <ipc/ipc.h>

struct vm;

int exec_load(struct vm *, vaddr_t *, const char *, fs_file_read_op_t *, fs_context_t, fs_file_context_t) __non_null(1, 2, 3, 4) __check_result;
int exec_task(ipc_port_t, ipc_port_t *, const char *, fs_file_read_op_t *, fs_context_t, fs_file_context_t) __non_null(3, 4) __check_result;

#endif /* !_CORE_EXEC_H_ */

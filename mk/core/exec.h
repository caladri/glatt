#ifndef	_CORE_EXEC_H_
#define	_CORE_EXEC_H_

#include <fs/fs_ops.h>

struct vm;

int exec_load(struct vm *, void **, const char *, fs_file_read_op_t *, fs_context_t, fs_file_context_t) __non_null(1, 2, 3, 4) __check_result;
int exec_task(const char *, fs_file_read_op_t *, fs_context_t, fs_file_context_t) __non_null(1, 2) __check_result;

#endif /* !_CORE_EXEC_H_ */

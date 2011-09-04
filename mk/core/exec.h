#ifndef	_CORE_EXEC_H_
#define	_CORE_EXEC_H_

struct task;

typedef	int exec_read_t(void *, void *, off_t, size_t *) __non_null(2, 4) __check_result;

int exec_load(struct task *, const char *, exec_read_t *, void *) __non_null(1, 2, 3) __check_result;
int exec_task(const char *, exec_read_t *, void *) __non_null(1, 2) __check_result;

#endif /* !_CORE_EXEC_H_ */

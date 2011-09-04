#ifndef	_CORE_EXEC_H_
#define	_CORE_EXEC_H_

struct task;

typedef	int exec_read_t(void *, void *, off_t, size_t *);

int exec_load(struct task *, const char *, exec_read_t *, void *) __check_result;
int exec_task(const char *, exec_read_t *, void *) __check_result;

#endif /* !_CORE_EXEC_H_ */
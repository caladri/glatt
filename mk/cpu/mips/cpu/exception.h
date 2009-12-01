#ifndef	_CPU_EXCEPTION_H_
#define	_CPU_EXCEPTION_H_

struct frame;

void cpu_exception_init(void);
void exception(struct frame *) __non_null(1);

#endif /* !_CPU_EXCEPTION_H_ */

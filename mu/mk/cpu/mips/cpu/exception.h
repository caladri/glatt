#ifndef	_CPU_EXCEPTION_H_
#define	_CPU_EXCEPTION_H_

struct frame;

void cpu_exception_init(void);
void exception(struct frame *);

#endif /* !_CPU_EXCEPTION_H_ */

#ifndef	_CORE_SLEEPQ_H_
#define	_CORE_SLEEPQ_H_

void sleepq_enter(const void *);
void sleepq_signal(const void *);
void sleepq_signal_one(const void *);
void sleepq_wait(const void *);

#endif /* !_CORE_SLEEPQ_H_ */

#ifndef	_CORE_CV_H_
#define	_CORE_CV_H_

struct cv;
struct mutex;

struct cv *cv_create(struct mutex *);
void cv_signal(struct cv *);
void cv_signal_broadcast(struct cv *);
void cv_wait(struct cv *);

#endif /* !_CORE_CV_H_ */

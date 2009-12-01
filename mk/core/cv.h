#ifndef	_CORE_CV_H_
#define	_CORE_CV_H_

struct cv;
struct mutex;

struct cv *cv_create(struct mutex *) __non_null(1);
void cv_signal(struct cv *) __non_null(1);
void cv_signal_broadcast(struct cv *) __non_null(1);
void cv_wait(struct cv *) __non_null(1);

#endif /* !_CORE_CV_H_ */

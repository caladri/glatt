#ifndef	_SUPERVISOR_INTERNAL_H_
#define	_SUPERVISOR_INTERNAL_H_

#define	SUPERVISOR_FUNCTIONS
#include <sk/supervisor.h>
#undef SUPERVISOR_FUNCTIONS

bool supervisor_invoke(enum FunctionIndex, void *, size_t);

#endif /* !_SUPERVISOR_INTERNAL_H_ */

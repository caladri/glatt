#ifndef	_DB_DB_H_
#define	_DB_DB_H_

#include <cpu/db.h>

#define	ASSERT(p, s)							\
	if (!(p))							\
		panic("%s:%u (in %s): %s failed: %s",			\
		      __FILE__, __LINE__, __func__, #p, s)

void panic(const char *, ...) __noreturn __printf(1, 2);

#endif /* !_DB_DB_H_ */

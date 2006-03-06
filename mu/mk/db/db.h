#ifndef	_DB_DB_H_
#define	_DB_DB_H_

#include <cpu/db.h>

#define	ASSERT(p, s)							\
	if (!(p))							\
		panic("%s:%u (in %s): %s failed: %s",			\
		      __FILE__, __LINE__, __func__, #p, s)

void panic(const char *, ...) __noreturn/*XXX %m __printf(1, 2)*/;

void db_enter(void) __noreturn;

#endif /* !_DB_DB_H_ */

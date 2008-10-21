#ifndef	_DB_DB_H_
#define	_DB_DB_H_

#ifndef DB
#error "Should not include this file unless the debugger is enabled."
#endif

void db_init(void);

void db_enter(void) __noreturn;

#endif /* !_DB_DB_H_ */

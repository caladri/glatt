#ifndef	_DB_DB_H_
#define	_DB_DB_H_

#ifndef DB
#error "Should not include this file unless the debugger is enabled."
#endif

void db_init(void);

void db_enter(void);
int db_getargs(char *, size_t, unsigned *, const char **, size_t, const char *) __non_null(1, 3, 4, 6);
int db_getline(char *, size_t) __non_null(1);

#endif /* !_DB_DB_H_ */

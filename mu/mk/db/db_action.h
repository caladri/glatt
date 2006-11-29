#ifndef	_DB_DB_ACTION_H_
#define	_DB_DB_ACTION_H_

struct db_action {
	const char *dba_char;
	const char *dba_help;
	void (*dba_function)(void);
};

#define	DB_ACTION(char, help, function)					\
	static struct db_action db_action_ ## char = {			\
		.dba_char = #char,					\
		.dba_help = help,					\
		.dba_function = function,				\
	};								\
	SET_ADD(db_actions, db_action_ ## char)

#endif /* !_DB_DB_ACTION_H_ */

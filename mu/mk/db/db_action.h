#ifndef	_DB_DB_ACTION_H_
#define	_DB_DB_ACTION_H_

struct db_action {
	const char *dba_char;
	const char *dba_help;
	void (*dba_function)(void);
	struct db_action *dba_alias;
};

#define	DB_ACTION(char, help, function)					\
	static struct db_action db_action_ ## char = {			\
		.dba_char = #char,					\
		.dba_help = help,					\
		.dba_function = function,				\
	};								\
	SET_ADD(db_actions, db_action_ ## char)

#define	DB_ACTION_ALIAS(char, alias)					\
	static struct db_action db_action_ ## char = {			\
		.dba_char = #char,					\
		.dba_alias = &db_action_ ## alias,			\
	};								\
	SET_ADD(db_actions, db_action_ ## char)

#endif /* !_DB_DB_ACTION_H_ */

#ifndef	_DB_COMMAND_H_
#define	_DB_COMMAND_H_

struct db_command {
	const char *dbc_command;
	void (*dbc_function)(void);
	const char *dbc_help;
};

#define	DB_COMMAND(name, func, help)					\
	static struct db_command db_command_struct_ ## name = {		\
		.dbc_command  = #name,					\
		.dbc_function = func,					\
		.dbc_help     = help,					\
	};								\
	SET_ADD(db_commands, db_command_struct_ ## name)

#endif /* !_DB_COMMAND_H_ */

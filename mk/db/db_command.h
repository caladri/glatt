#ifndef	_DB_DB_COMMAND_H_
#define	_DB_DB_COMMAND_H_

#ifndef DB
#error "Should not include this file unless the debugger is enabled."
#endif

#include <core/btree.h>

struct db_command;

struct db_command_tree {
	const char *ct_name;
	struct db_command_tree *ct_parent;
	BTREE_ROOT(struct db_command) ct_commands;
	BTREE_NODE(struct db_command_tree) ct_tree;
};

#define	DB_COMMAND_TREE_DECLARE(symbol)					\
	extern struct db_command_tree db_command_tree_ ## symbol

#define	DB_COMMAND_TREE_POINTER(symbol)					\
	(&_CONCAT(db_command_tree_, symbol))

DB_COMMAND_TREE_DECLARE(root);

enum db_command_type {
	DB_COMMAND_TYPE_TREE,
	DB_COMMAND_TYPE_VOIDF,
};

union db_command_union {
	struct db_command_tree *c_tree;
	void (*c_voidf)(void);
};

struct db_command {
	const char *c_name;
	struct db_command_tree *c_parent;
	enum db_command_type c_type;
	union db_command_union c_union;
	BTREE_NODE(struct db_command) c_tree;
};

#define	DB_COMMAND_TREE(name, parent, symbol)				\
	struct db_command_tree db_command_tree_ ## symbol = {		\
		.ct_name = #name,					\
		.ct_commands = BTREE_ROOT_INITIALIZER(),		\
		.ct_tree = BTREE_NODE_INITIALIZER(),			\
	};								\
	SET_ADD(db_command_trees, db_command_tree_ ## symbol);		\
									\
	struct db_command db_command_ ## parent ## _ ## name = {	\
		.c_name = #name,					\
		.c_parent = DB_COMMAND_TREE_POINTER(parent),		\
		.c_type = DB_COMMAND_TYPE_TREE,				\
		.c_union.c_tree = DB_COMMAND_TREE_POINTER(symbol),	\
		.c_tree = BTREE_NODE_INITIALIZER(),			\
	};								\
	SET_ADD(db_commands, db_command_ ## parent ## _ ## name)

#define	DB_COMMAND(name, parent, function)				\
	struct db_command db_command_ ## parent ## _ ## name = {	\
		.c_name = #name,					\
		.c_parent = DB_COMMAND_TREE_POINTER(parent),		\
		.c_type = DB_COMMAND_TYPE_VOIDF,			\
		.c_union.c_voidf = (function),				\
		.c_tree = BTREE_NODE_INITIALIZER(),			\
	};								\
	SET_ADD(db_commands, db_command_ ## parent ## _ ## name)

void db_command_init(void);

void db_command_enter(void);

#endif /* !_DB_DB_COMMAND_H_ */

#ifndef	_DB_DB_SHOW_H_
#define	_DB_DB_SHOW_H_

#include <core/queue.h>

struct db_show_value;

struct db_show_tree {
	const char *st_name;
	struct db_show_tree *st_parent;
	bool st_root;
	SLIST_HEAD(, struct db_show_value) st_values;
	SLIST_ENTRY(struct db_show_tree) st_link;
};

#define	DB_SHOW_TREE(name, root)					\
	struct db_show_tree db_show_tree_ ## name = 	{		\
		.st_name = #name,					\
		.st_root = root,					\
	};								\
	SET_ADD(db_show_trees, db_show_tree_ ## name)

#define	DB_SHOW_TREE_DECLARE(name)					\
	extern struct db_show_tree db_show_tree_ ## name

#define	DB_SHOW_TREE_POINTER(name)					\
	(&_CONCAT(db_show_tree_, name))

enum db_show_type {
	DB_SHOW_TYPE_TREE,
	DB_SHOW_TYPE_VOIDF,
};

union db_show_value_union {
	struct db_show_tree *sv_tree;
	void (*sv_voidf)(void);
};

struct db_show_value {
	const char *sv_name;
	struct db_show_tree *sv_parent;
	enum db_show_type sv_type;
	union db_show_value_union sv_value;
	SLIST_ENTRY(struct db_show_value) sv_link;
};

#define	DB_SHOW_VALUE_TREE(name, parent, value)				\
	struct db_show_value db_show_value_ ## parent ## _ ## name = {	\
		.sv_name = #name,					\
		.sv_parent = DB_SHOW_TREE_POINTER(parent),		\
		.sv_type = DB_SHOW_TYPE_TREE,				\
		.sv_value.sv_tree = value,				\
	};								\
	SET_ADD(db_show_values, db_show_value_ ## parent ## _ ## name)

#define	DB_SHOW_VALUE_VOIDF(name, parent, value)			\
	struct db_show_value db_show_value_ ## parent ## _ ## name = {	\
		.sv_name = #name,					\
		.sv_parent = DB_SHOW_TREE_POINTER(parent),		\
		.sv_type = DB_SHOW_TYPE_VOIDF,				\
		.sv_value.sv_voidf = value,				\
	};								\
	SET_ADD(db_show_values, db_show_value_ ## parent ## _ ## name)

void db_show_init(void);

#endif /* !_DB_DB_SHOW_H_ */

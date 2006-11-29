#ifndef	_DB_DB_SHOW_H_
#define	_DB_DB_SHOW_H_

#include <core/queue.h>

struct db_show_value;

struct db_show_tree {
	const char *st_name;
	bool st_root;
	SLIST_HEAD(, struct db_show_value) st_values;
	SLIST_ENTRY(struct db_show_tree) st_link;
};

#define	DB_SHOW_TREE(name, root)					\
	struct db_show_tree db_show_tree_ ## name = 	{		\
		.st_name = #name,					\
		.st_root = root,					\
		.st_values = SLIST_INIT(&db_show_tree_ ## name.st_values),\
	};								\
	SET_ADD(db_show_trees, db_show_tree_ ## name)

#define	DB_SHOW_TREE_DECLARE(name)					\
	extern struct db_show_tree db_show_tree_ ## name

#define	DB_SHOW_TREE_POINTER(name)					\
	(&_CONCAT(db_show_tree_, name))

enum db_show_type {
	DB_SHOW_TYPE_TREE,
	DB_SHOW_TYPE_INTF,
	DB_SHOW_TYPE_POINTERF,
	DB_SHOW_TYPE_STRINGF,
	DB_SHOW_TYPE_VOIDF,
};

struct db_show_value {
	const char *sv_name;
	struct db_show_tree *sv_parent;
	enum db_show_type sv_type;
	union {
		struct db_show_tree *sv_tree;
		int (*sv_intf)(void);
		void *(*sv_pointerf)(void);
		const char *(*sv_stringf)(void);
		void (*sv_voidf)(void);
	} sv_value;
	SLIST_ENTRY(struct db_show_value) sv_link;
};

#define	DB_SHOW_VALUE(name, parent, type, value)			\
	struct db_show_value db_show_value_ ## parent ## _ ## name = {	\
		.sv_name = #name,					\
		.sv_parent = DB_SHOW_TREE_POINTER(parent),		\
		.sv_type = type,					\
		.sv_value = value,					\
	};								\
	SET_ADD(db_show_values, db_show_value_ ## parent ## _ ## name)

#endif /* !_DB_DB_SHOW_H_ */

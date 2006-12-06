#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <db/db.h>
#include <db/db_action.h>
#include <db/db_show.h>
#include <io/device/console/console.h>

#define	DB_SHOW_ARG_MAX		(16)
#define	DB_SHOW_ARG_LIKELY_SIZE	(16)

SET(db_show_trees, struct db_show_tree);
SET(db_show_values, struct db_show_value);

static const char *db_show_argv[DB_SHOW_ARG_MAX];
static char db_show_buffer[DB_SHOW_ARG_MAX * DB_SHOW_ARG_LIKELY_SIZE];
static SLIST_HEAD(, struct db_show_tree) db_show_root;

static void db_show_all(struct db_show_tree *);
static bool db_show_change_current(struct db_show_tree **, const char *);
static int db_show_input(void);
static void db_show_listing(struct db_show_tree *);
static void db_show_listing_value(struct db_show_value *);
static bool db_show_one(struct db_show_tree **, const char *);
static void db_show_path(struct db_show_tree *);
static void db_show_value(struct db_show_value *);
static void db_show_value_ambiguous(struct db_show_tree *, const char *);
static struct db_show_value *db_show_value_lookup(struct db_show_tree *,
						  const char *);

void
db_show_init(void)
{
	struct db_show_tree *tree, **treep;
	struct db_show_value *value, **valuep;

	for (treep = SET_BEGIN(db_show_trees);
	     treep != SET_END(db_show_trees); treep++) {
		tree = *treep;
		if (tree->st_root)
			SLIST_INSERT_HEAD(&db_show_root, tree, st_link);
		SLIST_INIT(&tree->st_values);
	}
	for (valuep = SET_BEGIN(db_show_values);
	     valuep != SET_END(db_show_values); valuep++) {
		value = *valuep;
		SLIST_INSERT_HEAD(&value->sv_parent->st_values, value, sv_link);
		if (value->sv_type == DB_SHOW_TYPE_TREE)
			value->sv_value.sv_tree->st_parent = value->sv_parent;
	}
}

static void
db_show(void)
{
	struct db_show_tree *current;
	struct db_show_value *value;
	int argc, i;

	current = NULL;

	for (;;) {
		kcprintf("(db show ");
		db_show_path(current);
		kcprintf(") ");
		argc = db_show_input();
		if (argc == -1)
			return;
		for (i = 0; i < argc; i++) {
			if (strcmp(db_show_argv[i], "..") == 0) {
				if (!db_show_change_current(&current,
							    db_show_argv[i]))
					return;
			}
			if (strcmp(db_show_argv[i], "") == 0 ||
			    strcmp(db_show_argv[i], ".") == 0 ||
			    strcmp(db_show_argv[i], "?") == 0 ||
			    strcmp(db_show_argv[i], "help") == 0) {
				db_show_listing(current);
				continue;
			}
			if (strcmp(db_show_argv[i], "*") == 0) {
				db_show_all(current);
				continue;
			}
			/*
			 * A failure could indicate a failure to change position
			 * in the tree, in which case we should give up on this
			 * set of arguments.
			 */
			if (!db_show_one(&current, db_show_argv[i])) {
				kcprintf("DB: show failed.\n");
				break;
			}
		}
	}
}
DB_ACTION(s, "Explore a maze of twisty values!", db_show);

static void
db_show_all(struct db_show_tree *tree)
{
	struct db_show_value *value;

	if (tree == NULL) {
		db_show_listing(tree);
		return;
	}

	kcprintf("Values in ");
	db_show_path(tree);
	kcprintf(":\n");

	SLIST_FOREACH(value, &tree->st_values, sv_link) {
		kcprintf("%s:\n", value->sv_name);
		db_show_value(value);
	}
}

static bool
db_show_change_current(struct db_show_tree **currentp, const char *name)
{
	struct db_show_tree *tree;
	struct db_show_value *value;

	if (strcmp(name, "..") == 0) {
		if (*currentp == NULL)
			return (false);
		*currentp = (*currentp)->st_parent;
		db_show_listing(*currentp);
		return (true);
	}
	if (*currentp == NULL) {
		SLIST_FOREACH(tree, &db_show_root, st_link) {
			if (strcmp(tree->st_name, name) == 0) {
				*currentp = tree;
				db_show_listing(*currentp);
				return (true);
			}
		}
		return (false);
	}
	value = db_show_value_lookup(*currentp, name);
	if (value == NULL)
		return (false);
	if (value->sv_type != DB_SHOW_TYPE_TREE) {
		kcprintf("DB: cannot change current to non-tree %s!\n",
			 value->sv_name);
		return (false);
	}
	*currentp = value->sv_value.sv_tree;
	db_show_listing(*currentp);
	return (true);
}

static int
db_show_input(void)
{
	char ch;
	int argc;
	int error;
	unsigned c;

	argc = 0;

	db_show_argv[0] = db_show_buffer;

	for (c = 0; c < sizeof db_show_buffer; c++) {
		do {
			error = kcgetc(&db_show_buffer[c]);
		} while (error == ERROR_AGAIN);
		if (error != 0)
			return (-1);
		switch (db_show_buffer[c]) {
		case '\r':
		case '\n':
			db_show_buffer[c] = '\0';
			argc++;
			kcprintf("\n");
			return (argc);
		case '\t':
		case ' ':
		case '/':
			db_show_buffer[c] = '\0';
			db_show_argv[++argc] = &db_show_buffer[c + 1];
			kcputc(' ');
			break;
		default:
			kcputc(db_show_buffer[c]);
			break;
		}
	}
	kcprintf("DB: buffer overflow.\n");
	return (-1);
}

static void
db_show_listing(struct db_show_tree *tree)
{
	struct db_show_value *value;

	if (tree == NULL) {
		kcprintf("Available trees:");
		SLIST_FOREACH(tree, &db_show_root, st_link)
			kcprintf(" %s", tree->st_name);
		kcprintf("\n");
		return;
	}
	kcprintf("Available values:");
	SLIST_FOREACH(value, &tree->st_values, sv_link) {
		kcprintf(" ");
		db_show_listing_value(value);
	}
	kcprintf("\n");
}

static void
db_show_listing_value(struct db_show_value *value)
{
	char suffix;
	switch (value->sv_type) {
	case DB_SHOW_TYPE_TREE:
		suffix = '/';
		break;
	case DB_SHOW_TYPE_VOIDF:
		suffix = '*';
		break;
	default:
		suffix = '?';
	}
	kcprintf("%s%c", value->sv_name, suffix);
}

static void
db_show_path(struct db_show_tree *current)
{
	if (current == NULL) {
		kcprintf("(root)");
		return;
	}
	db_show_path(current->st_parent);
	kcprintf("/%s", current->st_name);
}

static bool
db_show_one(struct db_show_tree **currentp, const char *name)
{
	struct db_show_value *value;

	if (*currentp == NULL)
		return (db_show_change_current(currentp, name));
	value = db_show_value_lookup(*currentp, name);
	if (value == NULL)
		return (false);
	if (value->sv_type == DB_SHOW_TYPE_TREE)
		return (db_show_change_current(currentp, name));
	db_show_value(value);
	return (true);
}

static void
db_show_value(struct db_show_value *value)
{
	switch (value->sv_type) {
	case DB_SHOW_TYPE_VOIDF:
		value->sv_value.sv_voidf();
		break;
	default:
		panic("%s: unimplemented.\n", __func__);
	}
}

static void
db_show_value_ambiguous(struct db_show_tree *tree, const char *name)
{
	struct db_show_value *value;

	kcprintf("DB: ambiguous component '%s' possible matches:", name);

	SLIST_FOREACH(value, &tree->st_values, sv_link) {
		if (strlen(name) < strlen(value->sv_name)) {
			if (strncmp(name, value->sv_name, strlen(name)) != 0)
				continue;
			kcprintf(" %s", value->sv_name);
		}
	}
	kcprintf("\n");
}

static struct db_show_value *
db_show_value_lookup(struct db_show_tree *tree, const char *name)
{
	struct db_show_value *value, *match;
	bool ambiguous;

	ambiguous = false;
	match = NULL;

	SLIST_FOREACH(value, &tree->st_values, sv_link) {
		if (strlen(name) <= strlen(value->sv_name)) {
			if (strcmp(name, value->sv_name) == 0)
				return (value);
			/*
			 * There's a faster way, of course, but it doesn't
			 * matter.  In this case, you can juse do
			 * if (value->sv_name[strlen(name)] == '\0');
			 */
			if (strlen(name) == strlen(value->sv_name))
				continue;
			if (strncmp(name, value->sv_name, strlen(name)) == 0) {
				if (match != NULL) {
					ambiguous = true;
					continue;
				}
				match = value;
			}
		}
	}
	/*
	 * Delay this as we may have inexact matches before an exact match.
	 */
	if (ambiguous) {
		db_show_value_ambiguous(tree, name);
		return (NULL);
	}
	if (match != NULL)
		return (match);
	return (NULL);
}

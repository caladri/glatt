#include <core/types.h>
#include <core/btree.h>
#include <core/error.h>
#include <core/string.h>
#include <cpu/startup.h>
#include <db/db.h>
#include <db/db_command.h>
#include <core/console.h>

#define	DB_COMMAND_ARG_MAX		(16)
#define	DB_COMMAND_ARG_LIKELY_SIZE	(16)

SET(db_command_trees, struct db_command_tree);
SET(db_commands, struct db_command);

DB_COMMAND_TREE(root, root, root);

static bool db_command_return;
static char db_command_buffer[DB_COMMAND_ARG_MAX * DB_COMMAND_ARG_LIKELY_SIZE];

static void db_command_all(struct db_command_tree *);
static void db_command_listing(struct db_command_tree *);
static void db_command_listing_one(struct db_command *);
static bool db_command_one(struct db_command_tree **, const char *);
static void db_command_path(struct db_command_tree *);
static void db_command(struct db_command *, bool);
static void db_command_ambiguous(struct db_command_tree *, const char *);
static struct db_command *db_command_lookup(struct db_command_tree *,
					    const char *);

void
db_command_init(void)
{
	struct db_command **cmdp, *iter;

	/*
	 * Compile a command tree from the db_commands set.
	 */
	SET_FOREACH(cmdp, db_commands) {
		struct db_command *cmd = *cmdp;
		struct db_command_tree *parent = cmd->c_parent;

		/*
		 * The root is illusory, don't compile it in.
		 */
		if (cmd->c_type == DB_COMMAND_TYPE_TREE &&
		    cmd->c_union.c_tree == &db_command_tree_root)
			continue;

		BTREE_INSERT(cmd, iter, &parent->ct_commands, c_tree,
			     strcmp(cmd->c_name, iter->c_name) < 0);

		if (cmd->c_type == DB_COMMAND_TYPE_TREE)
			cmd->c_union.c_tree->ct_parent = parent;
	}
}

void
db_command_enter(void)
{
	const char *argv[DB_COMMAND_ARG_MAX];
	struct db_command_tree *current, *old;
	unsigned argc, i;
	int error;

	current = &db_command_tree_root;
	db_command_return = false;

	while (!db_command_return) {
		kcprintf("(db ");
		db_command_path(current);
		kcprintf(") ");


		error = db_getargs(db_command_buffer, sizeof db_command_buffer, &argc, argv,
				   DB_COMMAND_ARG_MAX, " \t/");
		if (error != 0) {
			kcprintf("\nDB: Input not available.  Halting.\n");
			cpu_halt();
		}

		old = current;

		for (i = 0; i < argc; i++) {
			if (strcmp(argv[i], "..") == 0) {
				current = current->ct_parent;
				continue;
			}
			if (strcmp(argv[i], "") == 0 ||
			    strcmp(argv[i], ".") == 0 ||
			    strcmp(argv[i], "?") == 0 ||
			    strcmp(argv[i], "help") == 0) {
				db_command_listing(current);
				continue;
			}
			if (strcmp(argv[i], "*") == 0) {
				db_command_all(current);
				continue;
			}
			/*
			 * A failure could indicate a failure to change position
			 * in the tree, in which case we should give up on this
			 * set of arguments.
			 */
			if (!db_command_one(&current, argv[i])) {
				kcprintf("DB: command failed.\n");
				current = old;
				break;
			}
		}
	}
}

static void
db_command_all(struct db_command_tree *tree)
{
	struct db_command *cmd;

	if (tree == NULL) {
		db_command_listing(tree);
		return;
	}

	kcprintf("cmds in ");
	db_command_path(tree);
	kcprintf(":\n");

	BTREE_FOREACH(cmd, &tree->ct_commands, c_tree,
		      db_command(cmd, true));
}

static void
db_command_listing(struct db_command_tree *tree)
{
	struct db_command *cmd;

	kcprintf("Available cmds:");
	BTREE_FOREACH(cmd, &tree->ct_commands, c_tree,
		      db_command_listing_one(cmd));
	kcprintf("\n");
}

static void
db_command_listing_one(struct db_command *cmd)
{
	char suffix;

	switch (cmd->c_type) {
	case DB_COMMAND_TYPE_TREE:
		suffix = '/';
		break;
	case DB_COMMAND_TYPE_VOIDF:
		suffix = '*';
		break;
	default:
		suffix = '?';
	}
	kcprintf(" %s%c", cmd->c_name, suffix);
}

static void
db_command_path(struct db_command_tree *current)
{
	if (current == &db_command_tree_root) {
		kcprintf("/");
		return;
	} else {
		db_command_path(current->ct_parent);
		kcprintf("%s/", current->ct_name);
	}
}

static bool
db_command_one(struct db_command_tree **currentp, const char *name)
{
	struct db_command *cmd;

	cmd = db_command_lookup(*currentp, name);
	if (cmd == NULL)
		return (false);
	if (cmd->c_type == DB_COMMAND_TYPE_TREE) {
		*currentp = cmd->c_union.c_tree;
		return (true);
	}
	db_command(cmd, false);
	return (true);
}

static void
db_command(struct db_command *cmd, bool show_name)
{
	if (show_name)
		kcprintf("%s:\n", cmd->c_name);

	switch (cmd->c_type) {
	case DB_COMMAND_TYPE_TREE:
		kcprintf("%s is a directory.\n", cmd->c_name);
		break;
	case DB_COMMAND_TYPE_VOIDF:
		cmd->c_union.c_voidf();
		break;
	default:
		NOTREACHED();
	}
}

static void
db_command_ambiguous(struct db_command_tree *tree, const char *name)
{
	struct db_command *cmd;

	kcprintf("DB: ambiguous component '%s' possible matches:", name);

	BTREE_MIN(cmd, &tree->ct_commands, c_tree);
	while (cmd != NULL) {
		if (strlen(name) < strlen(cmd->c_name)) {
			if (strncmp(name, cmd->c_name, strlen(name)) != 0) {
				BTREE_NEXT(cmd, c_tree);
				continue;
			}
			kcprintf(" %s", cmd->c_name);
		}
		BTREE_NEXT(cmd, c_tree);
	}
	kcprintf("\n");
}

static struct db_command *
db_command_lookup(struct db_command_tree *tree, const char *name)
{
	struct db_command *cmd, *match;
	bool ambiguous;

	ambiguous = false;
	match = NULL;

	BTREE_MIN(cmd, &tree->ct_commands, c_tree);
	while (cmd != NULL) {
		if (strlen(name) <= strlen(cmd->c_name)) {
			if (strcmp(name, cmd->c_name) == 0)
				return (cmd);
			/*
			 * There's a faster way, of course, but it doesn't
			 * matter.  In this case, you can juse do
			 * if (cmd->c_name[strlen(name)] == '\0');
			 */
			if (strlen(name) == strlen(cmd->c_name)) {
				BTREE_NEXT(cmd, c_tree);
				continue;
			}
			if (strncmp(name, cmd->c_name, strlen(name)) == 0) {
				if (match != NULL) {
					ambiguous = true;
					BTREE_NEXT(cmd, c_tree);
					continue;
				}
				match = cmd;
			}
		}
		BTREE_NEXT(cmd, c_tree);
	}

	/*
	 * Delay this as we may have inexact matches before an exact match.
	 */
	if (ambiguous) {
		db_command_ambiguous(tree, name);
		return (NULL);
	}
	if (match != NULL)
		return (match);
	return (NULL);
}

static void
db_command_exit(void)
{
	ASSERT(!db_command_return, "Can't leave debugger twice.");
	kcprintf("Leaving debugger.\n");
	db_command_return = true;
}
DB_COMMAND(exit, root, db_command_exit);
DB_COMMAND(quit, root, db_command_exit);

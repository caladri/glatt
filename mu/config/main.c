#include <sys/types.h>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char *options_required[] = {
	"std",
	NULL
};

struct file {
	const char *f_path;
	struct file *f_next;
};

struct option {
	const char *o_name;
	bool o_enable;
	struct file *o_files;
	struct option *o_next;
};

static void config(struct option *, const char *);
static void enable(struct option *, const char *, bool);
static struct option *load(struct option *, const char *, const char *);
static struct option *mkfile(struct option *, const char *, const char *);
static struct option *mkoption(struct option *, const char *);
static struct option *parse(struct option *, int, FILE *);
static void require(struct option *, const char *);
static struct option *search(struct option *, const char *);
static void show(struct option *);
static void usage(void);

int
main(int argc, char *argv[])
{
	const char *root, *platform, *configuration;
	struct option *options;
	const char **requirep;
	int ch;

	options = NULL;

	for (requirep = options_required; *requirep != NULL; requirep++)
		options = mkoption(options, *requirep);

	while ((ch = getopt(argc, argv, "")) != -1) {
		switch (ch) {
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 3)
		usage();

	root = *argv++;
	argc--;

	platform = *argv++;
	argc--;

	configuration = *argv++;
	argc--;

	options = load(options, root, platform);

	config(options, configuration);

	for (requirep = options_required; *requirep != NULL; requirep++)
		require(options, *requirep);
	
	show(options);

	return (0);
}

static void
config(struct option *options, const char *configuration)
{
	char name[1024];
	bool enabled;
	bool first;
	size_t offset;

	first = true;

	while (configuration[0] != '\0') {
		switch (configuration[0]) {
		case '-':
			configuration++;
			enabled = false;
			break;
		case '+':
			configuration++;
			enabled = true;
			break;
		default:
			if (!first)
				errx(1, "configuration must have a + or -");
			enabled = true;
			break;
		}
		first = false;

		offset = strcspn(configuration, "-+");
		if (offset == 0)
			errx(1, "extra - or + in configuration string");
		if (offset > sizeof name)
			errx(1, "name too long: %s", configuration);
		memcpy(name, configuration, offset);
		name[offset] = '\0';
		configuration += offset;

		if (enabled)
			enable(options, name, true);
		else
			enable(options, name, false);
	}
}

static void
enable(struct option *options, const char *name, bool yes)
{
	struct option *option;

	option = search(options, name);
	if (option == NULL)
		errx(1, "cannot set unknown option %s", name);
	option->o_enable = yes;
}

static struct option *
include(struct option *options, int rootdir, ...)
{
	const char *dir;
	va_list ap;
	FILE *file;
	int rv;

	rv = fchdir(rootdir);
	if (rv == -1)
		err(1, "could not enter kernel root directory");

	va_start(ap, rootdir);
	for (;;) {
		dir = va_arg(ap, const char *);
		if (dir == NULL)
			break;
		rv = chdir(dir);
		if (rv == -1)
			err(1, "could not enter subdirectory %s", dir);
	}
	va_end(ap);

	file = fopen("CONFIG", "r");
	if (file == NULL)
		errx(1, "open CONFIG");

	options = parse(options, rootdir, file);

	fclose(file);

	return (options);
}

static struct option *
load(struct option *options, const char *root, const char *platform)
{
	int rootdir;
	int srcdir;
	int rv;

	srcdir = open(".", O_RDONLY);
	if (srcdir == -1)
		err(1, "could not open current directory");

	rootdir = open(root, O_RDONLY);
	if (rootdir == -1)
		err(1, "could not open kernel root directory");

	options = include(options, rootdir, "platform", platform, NULL);
	options = include(options, rootdir, "build", NULL);

	rv = fchdir(srcdir);
	if (rv == -1)
		err(1, "could not return to last directory");

	close(srcdir);

	return (options);
}

static struct option *
mkfile(struct option *options, const char *name, const char *path)
{
	struct option *option;
	struct file *file;

	option = search(options, name);
	if (option == NULL)
		return (mkfile(mkoption(options, name), name, path));
	file = malloc(sizeof *file);
	file->f_path = strdup(path);
	file->f_next = option->o_files;
	option->o_files = file;
	return (options);
}

static struct option *
mkoption(struct option *options, const char *name)
{
	struct option *option;

	option = search(options, name);
	if (option != NULL)
		return (options);

	option = malloc(sizeof *option);
	if (option == NULL)
		errx(1, "malloc failed");
	option->o_name = strdup(name);
	option->o_enable = false;
	option->o_next = options;

	return (option);
}

static struct option *
parse(struct option *options, int rootdir, FILE *file)
{
	char name[1024];
	size_t offset;
	char *line;
	size_t len;

	while ((line = fgetln(file, &len)) != NULL) {
		if (len == 0)
			continue;
		line[len - 1] = '\0';

		if (line[0] == '#' || line[0] == '\0')
			continue;

		offset = strcspn(line, " \t");
		if (offset == 0)
			errx(1, "expected option in config file.");
		if (offset > sizeof name)
			errx(1, "name too long: %s", line);
		memcpy(name, line, offset);
		name[offset] = '\0';
		line += offset;

		while (isspace(*(unsigned char *)line))
			line++;

		if (strcasecmp(name, "cpu") == 0) {
			if (strcspn(line, " \t") != strlen(line))
				errx(1, "whitespace at end of cpu line");
			options = include(options, rootdir, "cpu", line, NULL);
		} else {
			if (strcspn(line, " \t") != strlen(line))
				errx(1, "whitespace at end of file line");
			options = mkfile(options, name, line);
		}
	}
	return (options);
}

static void
require(struct option *options, const char *name)
{
	struct option *option;

	option = search(options, name);
	if (option == NULL)
		errx(1, "required option %s not found", name);
	if (!option->o_enable)
		errx(1, "required option %s not set", name);
}

static struct option *
search(struct option *options, const char *name)
{
	struct option *option;

	for (option = options; option != NULL; option = option->o_next)
		if (strcasecmp(option->o_name, name) == 0)
			return (option);
	return (NULL);
}

static void
show(struct option *options)
{
	struct option *option;

	printf("configuration:\n");
	for (option = options; option != NULL; option = option->o_next) {
		printf("\t%c%s\n", option->o_enable ? '+' : '-',
		       option->o_name);
		if (option->o_files != NULL) {
			struct file *file;

			for (file = option->o_files; file != NULL;
			     file = file->f_next)
				printf("\t\t%s\n", file->f_path);
		} else {
			printf("\t\tno files\n");
		}
	}
}

static void
usage(void)
{
	fprintf(stderr,
"usage: mu-config kernel-base platform configuration\n");
	exit(1);
}

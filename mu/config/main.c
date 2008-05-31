#include <sys/types.h>
#include <err.h>
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

struct option {
	const char *o_name;
	bool o_enable;
	struct option *o_next;
};

static void config(struct option *, const char *);
static void enable(struct option *, const char *, bool);
static struct option *mkoption(struct option *, const char *);
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
	for (option = options; option != NULL; option = option->o_next)
		printf("\t%c%s\n", option->o_enable ? '+' : '-', option->o_name);
}

static void
usage(void)
{
	fprintf(stderr,
"usage: mu-config kernel-base platform configuration\n");
	exit(1);
}

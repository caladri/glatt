#include <sys/types.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
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

struct configuration {
	struct option *c_option;
	bool c_enable;
	struct configuration *c_next;
};

struct requirement {
	enum requirement_type {
		Implies,
		Requires,
		MutuallyExcludes,
	} r_type;
	struct option *r_option;
	struct requirement *r_next;
};

struct file {
	const char *f_path;
	bool f_onenable;
	struct file *f_next;
};

struct option {
	const char *o_name;
	bool o_enable;
	struct file *o_files;
	struct requirement *o_requirements;
	struct option *o_next;
};

struct options {
	struct option *o_options;
	const char *o_cpu;
};

static void check(struct options *);
static void config(struct options *, const char *);
static void generate(struct options *, const char *, const char *,
		     const char *);
static void imply(struct options *);
static void include(struct options *, int, ...);
static void load(struct options *, const char *, const char *);
static struct configuration *mkconfiguration(struct configuration *,
					     struct options *, const char *,
					     bool);
static void mkfile(struct options *, const char *, const char *, bool);
static void mkoption(struct options *, const char *);
static void mkrequirement(struct options *, const char *, enum requirement_type,
			  const char *);
static void parse(struct options *, int, FILE *);
static void require(struct options *, const char *);
static struct option *search(struct options *, const char *);
static void show(struct options *);
static void usage(void);

int
main(int argc, char *argv[])
{
	const char *root, *platform, *configuration;
	struct options options;
	bool doshow;
	int ch;

	options.o_options = NULL;
	options.o_cpu = NULL;

	doshow = false;

	while ((ch = getopt(argc, argv, "s")) != -1) {
		switch (ch) {
		case 's':
			doshow = true;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();
	root = *argv++;
	argc--;

	if (argc == 0)
		usage();
	platform = *argv++;
	argc--;

	if (argc == 0) {
		if (!doshow)
			usage();
		configuration = "std";
	} else {
		configuration = *argv++;
		argc--;
	}

	if (argc != 0)
		usage();

	load(&options, root, platform);
	config(&options, configuration);
	check(&options);

	if (doshow) {
		show(&options);
	} else {
		generate(&options, root, platform, configuration);
	}

	return (0);
}

static void
check(struct options *options)
{
	struct requirement *requirement;
	struct option *option;
	const char **requirep;

	if (options->o_cpu == NULL)
		errx(1, "must have a cpu.");

	for (requirep = options_required; *requirep != NULL; requirep++)
		require(options, *requirep);

	for (option = options->o_options; option != NULL;
	     option = option->o_next) {
		for (requirement = option->o_requirements; requirement != NULL;
		     requirement = requirement->r_next) {
			switch (requirement->r_type) {
			case Implies:
				if (!option->o_enable)
					continue;
				if (!requirement->r_option->o_enable) {
					errx(1, "%s should be enabled for %s",
					     requirement->r_option->o_name,
					     option->o_name);
				}
				continue;
			case Requires:
				if (!option->o_enable)
					continue;
				if (!requirement->r_option->o_enable) {
					errx(1, "%s must be enabled for %s",
					     requirement->r_option->o_name,
					     option->o_name);
				}
				continue;
			case MutuallyExcludes:
				if (!option->o_enable)
					continue;
				if (requirement->r_option->o_enable) {
					errx(1, "%s cannot be enabled with %s",
					     requirement->r_option->o_name,
					     option->o_name);
				}
				continue;
			}
		}
	}
}

static void
config(struct options *options, const char *configstr)
{
	struct configuration *configuration, *configurations;
	char name[1024];
	size_t offset;
	bool enabled;
	bool first;

	configurations = NULL;
	first = true;

	while (configstr[0] != '\0') {
		switch (configstr[0]) {
		case '-':
			configstr++;
			enabled = false;
			break;
		case '+':
			configstr++;
			enabled = true;
			break;
		default:
			if (!first)
				errx(1, "configuration must have a + or -");
			enabled = true;
			break;
		}
		first = false;

		offset = strcspn(configstr, "-+");
		if (offset == 0)
			errx(1, "extra - or + in configuration string");
		if (offset > sizeof name)
			errx(1, "name too long: %s", configstr);
		memcpy(name, configstr, offset);
		name[offset] = '\0';
		configstr += offset;

		configurations = mkconfiguration(configurations, options,
						 name, enabled);
	}

	/*
	 * First turn on things explicitly enabled by the user, then turn on
	 * implicit things, then disable things disabled by the user.
	 */
	for (configuration = configurations; configuration != NULL;
	     configuration = configuration->c_next) {
		if (configuration->c_enable)
			configuration->c_option->o_enable = true;
	}

	imply(options);

	for (configuration = configurations; configuration != NULL;
	     configuration = configuration->c_next) {
		if (!configuration->c_enable)
			configuration->c_option->o_enable = false;
	}

	while (configurations != NULL) {
		struct configuration *next = configurations->c_next;
		free(configurations);
		configurations = next;
	}
}

static void
include(struct options *options, int rootdir, ...)
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

	parse(options, rootdir, file);

	fclose(file);
}

static void
generate(struct options *options, const char *root, const char *platform,
	 const char *configstr)
{
	struct option *option;
	FILE *config_mk;
	int rv;

	rv = unlink("_root");
	if (rv == -1 && errno != ENOENT)
		err(1, "unable to remove _root symlink");

	rv = symlink(root, "_root");
	if (rv == -1)
		err(1, "unable to create _root symlink");

	rv = unlink("Makefile");
	if (rv == -1 && errno != ENOENT)
		err(1, "unable to remove Makefile symlink");

	rv = symlink("_root/build/Makefile", "Makefile");
	if (rv == -1)
		err(1, "unable to create Makefile symlink");

	config_mk = fopen("config.mk", "w");
	if (config_mk == NULL)
		err(1, "unable to open config.mk for writing.");

	fprintf(config_mk, "KERNEL_ROOT=%s\n", root);
	fprintf(config_mk, "PLATFORM=%s\n", platform);
	fprintf(config_mk, "CPU=%s\n", options->o_cpu);
	fprintf(config_mk, "CONFIGSTR=%s\n", configstr);

	for (option = options->o_options; option != NULL;
	     option = option->o_next) {
		struct file *file;
		char uppercase[1024], *q;
		const char *p;

		for (p = option->o_name, q = uppercase; *p != '\0'; p++, q++)
			*q = toupper(*p);
		*q = '\0';

		/* Write config.mk entry.  */
		if (option->o_enable)
			fprintf(config_mk, "%s=ENABLED\n", uppercase);
		else
			fprintf(config_mk, ".undef %s\n", uppercase);

		for (file = option->o_files; file != NULL;
		     file = file->f_next) {
			if (file->f_onenable != option->o_enable)
				continue;
			fprintf(config_mk, ".PATH: %s/%s\n",
				root, dirname(file->f_path));
			fprintf(config_mk, "KERNEL_SOURCES+=%s\n",
				basename(file->f_path));
		}
	}

	fclose(config_mk);
}

static void
imply(struct options *options)
{
	struct requirement *requirement;
	struct option *option;

restart:
	for (option = options->o_options; option != NULL;
	     option = option->o_next) {
		for (requirement = option->o_requirements; requirement != NULL;
		     requirement = requirement->r_next) {
			if (requirement->r_type != Implies)
				continue;
			if (!option->o_enable)
				continue;
			if (requirement->r_option->o_enable)
				continue;
			requirement->r_option->o_enable = true;
			goto restart;
		}
	}
}

static void
load(struct options *options, const char *root, const char *platform)
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

	include(options, rootdir, "platform", platform, NULL);
	include(options, rootdir, "build", NULL);

	rv = fchdir(srcdir);
	if (rv == -1)
		err(1, "could not return to last directory");

	close(srcdir);
}

static struct configuration *
mkconfiguration(struct configuration *configurations, struct options *options,
		const char *name, bool enabled)
{
	struct configuration *configuration;
	struct option *option;

	option = search(options, name);
	if (option == NULL)
		errx(1, "cannot configure non-existant option %s", name);

	for (configuration = configurations; configuration != NULL;
	     configuration = configuration->c_next) {
		if (configuration->c_option == option) {
			configuration->c_enable = enabled;
			return (configurations);
		}
	}

	configuration = malloc(sizeof *configuration);
	configuration->c_option = option;
	configuration->c_enable = enabled;
	configuration->c_next = configurations;
	configurations = configuration;
	return (configurations);
}

static void
mkfile(struct options *options, const char *name, const char *path,
       bool onenable)
{
	struct option *option;
	struct file *file;

	option = search(options, name);
	if (option == NULL) {
		mkoption(options, name);
		mkfile(options, name, path, onenable);
		return;
	}

	file = malloc(sizeof *file);
	file->f_path = strdup(path);
	file->f_onenable = onenable;
	file->f_next = option->o_files;
	option->o_files = file;
}

static void
mkoption(struct options *options, const char *name)
{
	struct option *option;

	option = search(options, name);
	if (option != NULL)
		return;

	option = malloc(sizeof *option);
	option->o_name = strdup(name);
	option->o_enable = false;
	option->o_files = NULL;
	option->o_requirements = NULL;
	option->o_next = options->o_options;
	options->o_options = option;
}

static void
mkrequirement(struct options *options, const char *name1,
	      enum requirement_type type, const char *name2)
{
	struct option *option1, *option2;
	struct requirement *requirement;

	option1 = search(options, name1);
	option2 = search(options, name2);

	if (option1 == NULL || option2 == NULL) {
		mkoption(options, name1);
		mkoption(options, name2);
		mkrequirement(options, name1, type, name2);
		return;
	}

	if (option1 == option2)
		errx(1, "option has requirement relationship with itself.");

	requirement = malloc(sizeof *requirement);
	requirement->r_type = type;
	requirement->r_option = option2;
	requirement->r_next = option1->o_requirements;
	option1->o_requirements = requirement;
}

static void
parse(struct options *options, int rootdir, FILE *file)
{
	char name[1024];
	bool onenable;
	size_t offset;
	char *line;
	size_t len;

	while ((line = fgetln(file, &len)) != NULL) {
		if (len == 0)
			continue;
		line[len - 1] = '\0';

		if (line[0] == '#' || line[0] == '\0')
			continue;

		onenable = true;
		if (line[0] == '!') {
			onenable = false;
			line++;
		}

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
			if (options->o_cpu != NULL)
				errx(1, "duplicate \"cpu\" directive.");
			options->o_cpu = strdup(line);
			include(options, rootdir, "cpu", options->o_cpu, NULL);
		} else {
			struct {
				const char *rm_prefix;
				enum requirement_type rm_type;
			} requirement_mapping[] = {
				{ "implies:",		Implies },
				{ "requires:",		Requires },
				{ "mutually-excludes:",	MutuallyExcludes },
				{ NULL,			0 /*XXX*/ },
			}, *rm;

			for (rm = requirement_mapping; rm->rm_prefix != NULL;
			     rm++) {
				if (strncmp(line, rm->rm_prefix,
					    strlen(rm->rm_prefix)) != 0)
					continue;
				if (!onenable)
					errx(1, "cannot invert a requirement");
				line += strlen(rm->rm_prefix);
				while (isspace(*(unsigned char *)line))
					line++;
				if (strcspn(line, " \t") != strlen(line))
					errx(1, "whitespace after requirement");
				mkrequirement(options, name, rm->rm_type, line);
				break;
			}
			if (rm->rm_prefix != NULL)
				continue;

			if (strcspn(line, " \t") != strlen(line))
				errx(1, "whitespace at end of file line");

			mkfile(options, name, line, onenable);
		}
	}
}

static void
require(struct options *options, const char *name)
{
	struct option *option;

	option = search(options, name);
	if (option == NULL)
		errx(1, "required option %s not found", name);
	if (!option->o_enable)
		errx(1, "required option %s not set", name);
}

static struct option *
search(struct options *options, const char *name)
{
	struct option *option;

	for (option = options->o_options; option != NULL;
	     option = option->o_next)
		if (strcasecmp(option->o_name, name) == 0)
			return (option);
	return (NULL);
}

static void
show(struct options *options)
{
	struct option *option;

	printf("configuration:\n");
	for (option = options->o_options; option != NULL;
	     option = option->o_next) {
		printf("\t%c%s\n", option->o_enable ? '+' : '-',
		       option->o_name);
		if (option->o_requirements != NULL) {
			struct requirement *requirement;

			printf("\t\trequirements:\n");
			for (requirement = option->o_requirements;
			     requirement != NULL;
			     requirement = requirement->r_next) {
				switch (requirement->r_type) {
				case Implies:
					printf("\t\t\timplies %s\n",
					       requirement->r_option->o_name);
					break;
				case Requires:
					printf("\t\t\trequires %s\n",
					       requirement->r_option->o_name);
					break;
				case MutuallyExcludes:
					printf("\t\t\tincompatible with %s\n",
					       requirement->r_option->o_name);
					break;
				}
			}
		} else {
			printf("\t\tno requirements\n");
		}
		if (option->o_files != NULL) {
			struct file *file;

			printf("\t\tfiles:\n");
			for (file = option->o_files; file != NULL;
			     file = file->f_next)
				printf("\t\t\t%s%s\n", file->f_onenable ?
				       "on enable " : "on disable ",
				       file->f_path);
		} else {
			printf("\t\tno files\n");
		}
	}
}

static void
usage(void)
{
	fprintf(stderr,
"usage: mu-config kernel-root platform configuration\n"
"       mu-config -s kernel-root platform [configuration]\n");
	exit(1);
}

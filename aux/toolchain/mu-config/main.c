#include <sys/types.h>
#include <ctype.h>
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

static const char *conf_required[] = {
	"std",
	NULL
};

struct setting {
	struct option *s_option;
	bool s_enable;
	struct setting *s_next;
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

struct configuration {
	struct option *c_options;
	struct setting *c_settings;
	const char *c_root;
	const char *c_platform;
	const char *c_configstr;
	const char *c_cpu;
};

static void check(struct configuration *);
static void config(struct configuration *);
static void fatal(struct configuration *, int, const char *, ...) __attribute__ ((__noreturn__));
static void generate(struct configuration *);
static const char *getsline(FILE *);
static void imply(struct configuration *);
static void include(struct configuration *, int, ...);
static void load(struct configuration *);
static void mkfile(struct configuration *, const char *, const char *, bool);
static void mkoption(struct configuration *, const char *);
static void mkrequirement(struct configuration *, const char *,
			  enum requirement_type, const char *);
static void mksetting(struct configuration *, const char *, bool);
static void parse(struct configuration *, int, FILE *);
static void require(struct configuration *, const char *);
static struct option *search(struct configuration *, const char *);
static void show(struct configuration *);
static void usage(void);

int
main(int argc, char *argv[])
{
	struct configuration conf;
	bool doshow;
	int ch;

	conf.c_options = NULL;
	conf.c_settings = NULL;
	conf.c_root = NULL;
	conf.c_platform = NULL;
	conf.c_configstr = "std";
	conf.c_cpu = NULL;

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
	conf.c_root = *argv++;
	argc--;

	if (argc == 0)
		usage();
	conf.c_platform = *argv++;
	argc--;

	if (argc == 0) {
		if (!doshow)
			usage();
	} else {
		conf.c_configstr = *argv++;
		argc--;
	}

	if (argc != 0)
		usage();

	load(&conf);
	config(&conf);
	check(&conf);

	if (doshow) {
		show(&conf);
	} else {
		generate(&conf);
	}

	return (0);
}

static void
check(struct configuration *conf)
{
	struct requirement *requirement;
	struct option *option;
	const char **requirep;

	if (conf->c_cpu == NULL)
		fatal(conf, 0, "must have a cpu.");

	for (requirep = conf_required; *requirep != NULL; requirep++)
		require(conf, *requirep);

	for (option = conf->c_options; option != NULL;
	     option = option->o_next) {
		for (requirement = option->o_requirements; requirement != NULL;
		     requirement = requirement->r_next) {
			switch (requirement->r_type) {
			case Implies:
				continue;
			case Requires:
				if (!option->o_enable)
					continue;
				if (!requirement->r_option->o_enable) {
					fatal(conf, 0, "%s must be enabled for %s",
					     requirement->r_option->o_name,
					     option->o_name);
				}
				continue;
			case MutuallyExcludes:
				if (!option->o_enable)
					continue;
				if (requirement->r_option->o_enable) {
					fatal(conf, 0, "%s cannot be enabled with %s",
					     requirement->r_option->o_name,
					     option->o_name);
				}
				continue;
			}
		}
	}
}

static void
config(struct configuration *conf)
{
	struct setting *setting;
	const char *configstr;
	char name[1024];
	size_t offset;
	bool enabled;
	bool first;

	first = true;
	configstr = conf->c_configstr;

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
				fatal(conf, 0, "configuration must have a + or -");
			enabled = true;
			break;
		}
		first = false;

		offset = strcspn(configstr, "-+");
		if (offset == 0)
			fatal(conf, 0, "extra - or + in configuration string");
		if (offset > sizeof name)
			fatal(conf, 0, "name too long: %s", configstr);
		memcpy(name, configstr, offset);
		name[offset] = '\0';
		configstr += offset;

		mksetting(conf, name, enabled);
	}

	/*
	 * First turn on things explicitly enabled by the user.  Second,
	 * disable anything the user requested be disabled.  Third, turn on
	 * anything implicitly-enabled by the remaining options.  Fourth,
	 * disable anything which was turned on implicitly that the user
	 * requested be disabled.
	 *
	 * NB: The first and second steps are both covered by this first loop.
	 */
	for (setting = conf->c_settings; setting != NULL;
	     setting = setting->s_next) {
		setting->s_option->o_enable = setting->s_enable;
	}

	imply(conf);

	for (setting = conf->c_settings; setting != NULL;
	     setting = setting->s_next) {
		if (!setting->s_enable)
			setting->s_option->o_enable = false;
	}
}

static void
include(struct configuration *conf, int rootdir, ...)
{
	const char *dir;
	va_list ap;
	FILE *file;
	int rv;

	rv = fchdir(rootdir);
	if (rv == -1)
		fatal(conf, errno, "could not enter kernel root directory");

	va_start(ap, rootdir);
	for (;;) {
		dir = va_arg(ap, const char *);
		if (dir == NULL)
			break;
		rv = chdir(dir);
		if (rv == -1)
			fatal(conf, errno, "could not enter subdirectory %s",
			      dir);
	}
	va_end(ap);

	file = fopen("CONFIG", "r");
	if (file == NULL)
		fatal(conf, 0, "open CONFIG");

	parse(conf, rootdir, file);

	fclose(file);
}

static void
fatal(struct configuration *conf, int error, const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "Fatal error.\n");
	fprintf(stderr, "mu-config: ");
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (error != 0)
		fprintf(stderr, ": %s\n", strerror(error));
	else
		fprintf(stderr, "\n");

	fprintf(stderr, "\troot\t\t%s\n", conf->c_root);
	fprintf(stderr, "\tplatform\t%s\n", conf->c_platform);
	fprintf(stderr, "\tconfigstr\t%s\n", conf->c_configstr);
	fprintf(stderr, "\tcpu\t\t%s\n", conf->c_cpu);

	exit(1);
}

static void
generate(struct configuration *conf)
{
	struct option *option;
	FILE *config_mk;
	int rv;

	rv = unlink("_root");
	if (rv == -1 && errno != ENOENT)
		fatal(conf, errno, "unable to remove _root symlink");

	rv = symlink(conf->c_root, "_root");
	if (rv == -1)
		fatal(conf, errno, "unable to create _root symlink");

	rv = unlink("Makefile");
	if (rv == -1 && errno != ENOENT)
		fatal(conf, errno, "unable to remove Makefile symlink");

	rv = symlink("_root/build/Makefile", "Makefile");
	if (rv == -1)
		fatal(conf, errno, "unable to create Makefile symlink");

	config_mk = fopen("config.mk", "w");
	if (config_mk == NULL)
		fatal(conf, errno, "unable to open config.mk for writing");

	fprintf(config_mk, "KERNEL_ROOT=%s\n", conf->c_root);
	fprintf(config_mk, "PLATFORM=%s\n", conf->c_platform);
	fprintf(config_mk, "CPU=%s\n", conf->c_cpu);
	fprintf(config_mk, "CONFIGSTR=%s\n", conf->c_configstr);

	for (option = conf->c_options; option != NULL;
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
				conf->c_root, dirname(file->f_path));
			fprintf(config_mk, "KERNEL_SOURCES+=%s\n",
				basename(file->f_path));
		}
	}

	fclose(config_mk);
}

static const char *
getsline(FILE *input)
{
	static char buf[1024];
	char *p;

	p = fgets(buf, sizeof buf, input);
	if (p == NULL)
		return (NULL);
	p = strchr(buf, '\n');
	if (p != NULL)
		*p = '\0';
	return (buf);
}

static void
imply(struct configuration *conf)
{
	struct requirement *requirement;
	struct option *option;

restart:
	for (option = conf->c_options; option != NULL;
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
load(struct configuration *conf)
{
	int rootdir;
	int srcdir;
	int rv;

	srcdir = open(".", O_RDONLY);
	if (srcdir == -1)
		fatal(conf, errno, "could not open current directory");

	rootdir = open(conf->c_root, O_RDONLY);
	if (rootdir == -1)
		fatal(conf, errno, "could not open kernel root directory");

	include(conf, rootdir, "platform", conf->c_platform, NULL);
	include(conf, rootdir, "build", NULL);

	rv = fchdir(srcdir);
	if (rv == -1)
		fatal(conf, errno, "could not return to last directory");

	close(srcdir);
}

static void
mkfile(struct configuration *conf, const char *name, const char *path,
       bool onenable)
{
	struct option *option;
	struct file *file;

	option = search(conf, name);
	if (option == NULL) {
		mkoption(conf, name);
		mkfile(conf, name, path, onenable);
		return;
	}

	file = malloc(sizeof *file);
	file->f_path = strdup(path);
	file->f_onenable = onenable;
	file->f_next = option->o_files;
	option->o_files = file;
}

static void
mkoption(struct configuration *conf, const char *name)
{
	struct option *option;

	option = search(conf, name);
	if (option != NULL)
		return;

	option = malloc(sizeof *option);
	option->o_name = strdup(name);
	option->o_enable = false;
	option->o_files = NULL;
	option->o_requirements = NULL;
	option->o_next = conf->c_options;
	conf->c_options = option;
}

static void
mkrequirement(struct configuration *conf, const char *name1,
	      enum requirement_type type, const char *name2)
{
	struct option *option1, *option2;
	struct requirement *requirement;

	option1 = search(conf, name1);
	option2 = search(conf, name2);

	if (option1 == NULL || option2 == NULL) {
		mkoption(conf, name1);
		mkoption(conf, name2);
		mkrequirement(conf, name1, type, name2);
		return;
	}

	if (option1 == option2)
		fatal(conf, 0, "option has requirement relationship with itself.");

	requirement = malloc(sizeof *requirement);
	requirement->r_type = type;
	requirement->r_option = option2;
	requirement->r_next = option1->o_requirements;
	option1->o_requirements = requirement;
}

static void
mksetting(struct configuration *conf, const char *name, bool enabled)
{
	struct setting *setting;
	struct option *option;

	option = search(conf, name);
	if (option == NULL)
		fatal(conf, 0, "cannot configure non-existant option %s", name);

	for (setting = conf->c_settings; setting != NULL;
	     setting = setting->s_next) {
		if (setting->s_option == option) {
			setting->s_enable = enabled;
			return;
		}
	}

	setting = malloc(sizeof *setting);
	setting->s_option = option;
	setting->s_enable = enabled;
	setting->s_next = conf->c_settings;
	conf->c_settings = setting;
}

static void
parse(struct configuration *conf, int rootdir, FILE *file)
{
	char name[1024];
	bool onenable;
	size_t offset;
	const char *line;

	while ((line = getsline(file)) != NULL) {
		if (line[0] == '#' || line[0] == '\0')
			continue;

		onenable = true;
		if (line[0] == '!') {
			onenable = false;
			line++;
		}

		offset = strcspn(line, " \t");
		if (offset == 0)
			fatal(conf, 0, "expected option in config file.");
		if (offset > sizeof name)
			fatal(conf, 0, "name too long: %s", line);
		memcpy(name, line, offset);
		name[offset] = '\0';
		line += offset;

		while (isspace(*(const unsigned char *)line))
			line++;

		if (strcasecmp(name, "cpu") == 0) {
			if (strcspn(line, " \t") != strlen(line))
				fatal(conf, 0, "whitespace at end of cpu line");
			if (conf->c_cpu != NULL)
				fatal(conf, 0, "duplicate \"cpu\" directive.");
			conf->c_cpu = strdup(line);
			include(conf, rootdir, "cpu", conf->c_cpu, NULL);
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
					fatal(conf, 0, "cannot invert a requirement");
				line += strlen(rm->rm_prefix);
				while (isspace(*(const unsigned char *)line))
					line++;
				if (strcspn(line, " \t") != strlen(line))
					fatal(conf, 0, "whitespace after requirement");
				mkrequirement(conf, name, rm->rm_type, line);
				break;
			}
			if (rm->rm_prefix != NULL)
				continue;

			if (strcspn(line, " \t") != strlen(line))
				fatal(conf, 0, "whitespace at end of file line");

			mkfile(conf, name, line, onenable);
		}
	}
	if (ferror(file))
		fatal(conf, errno, "error while parsing file");
}

static void
require(struct configuration *conf, const char *name)
{
	struct option *option;

	option = search(conf, name);
	if (option == NULL)
		fatal(conf, 0, "required option %s not found", name);
	if (!option->o_enable)
		fatal(conf, 0, "required option %s not set", name);
}

static struct option *
search(struct configuration *conf, const char *name)
{
	struct option *option;

	for (option = conf->c_options; option != NULL; option = option->o_next)
		if (strcasecmp(option->o_name, name) == 0)
			return (option);
	return (NULL);
}

static void
show(struct configuration *conf)
{
	struct option *option;

	printf("configuration:\n");
	for (option = conf->c_options; option != NULL;
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
"usage: mu-config kernel-root platform configstr\n"
"       mu-config -s kernel-root platform [configstr]\n");
	exit(1);
}

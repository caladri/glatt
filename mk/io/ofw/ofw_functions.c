#include <core/types.h>
#include <core/error.h>
#include <io/ofw/ofw.h>
#include <io/ofw/ofw_functions.h>

#define	OFW_ARGS_MAX	12
#define	OFW_RETURNS_MAX	1

static int ofw_function(const char *, ...);

int
ofw_exit(void)
{
	int error;

	error = ofw_function("exit", 0, 0);
	if (error != 0)
		panic("%s: error from ofw!", __func__);
	NOTREACHED();
}

int
ofw_finddevice(const char *device, ofw_package_t *packagep)
{
	ofw_word_t package;
	int error;

	error = ofw_function("finddevice",
			     1, (ofw_word_t)device,
			     1, &package);
	if (error != 0)
		return (error);

	*packagep = package;
	return (0);
}

int
ofw_getprop(ofw_package_t package, const char *propname, void *propval,
	    size_t propvallen, size_t *proplenp)
{
	ofw_word_t proplen;
	int error;

	error = ofw_function("getprop",
			     4, (ofw_word_t)package, (ofw_word_t)propname,
			        (ofw_word_t)propval, (ofw_word_t)propvallen,
			     1, &proplen);
	if (error != 0)
		return (error);

	*proplenp = proplen;
	return (0);
}

int
ofw_getproplen(ofw_package_t package, const char *propname, size_t *proplenp)
{
	ofw_word_t proplen;
	int error;

	error = ofw_function("getproplen",
			     2, (ofw_word_t)package, (ofw_word_t)propname,
			     1, &proplen);
	if (error != 0)
		return (error);

	*proplenp = proplen;
	return (0);
}

int
ofw_open(const char *name, ofw_instance_t *instancep)
{
	ofw_word_t instance;
	int error;

	error = ofw_function("open",
			     1, (ofw_word_t)name,
			     1, &instance);
	if (error != 0)
		return (error);

	*instancep = instance;
	return (0);
}

int
ofw_read(ofw_instance_t instance, void *buf, size_t buflen, size_t *lenp)
{
	ofw_word_t len;
	int error;

	error = ofw_function("read",
			     3, (ofw_word_t)instance, (ofw_word_t)buf,
			        (ofw_word_t)buflen,
			     1, &len);
	if (error != 0)
		return (error);

	*lenp = len;
	return (0);
}

int
ofw_write(ofw_instance_t instance, const void *buf, size_t buflen, size_t *lenp)
{
	ofw_word_t len;
	int error;

	error = ofw_function("write",
			     3, (ofw_word_t)instance, (ofw_word_t)buf,
			        (ofw_word_t)buflen,
			     1, &len);
	if (error != 0)
		return (error);

	*lenp = len;
	return (0);
}

static int
ofw_function(const char *function, ...)
{
	static ofw_word_t params[1 + OFW_ARGS_MAX + OFW_RETURNS_MAX];
	unsigned args, returns, param;
	ofw_return_t ret;
	ofw_word_t *outp;
	va_list ap;

	params[0] = (ofw_word_t)function;

	args = 0;
	returns = 0;

	va_start(ap, function);

	args = va_arg(ap, unsigned);
	if (args > OFW_ARGS_MAX) {
		va_end(ap);
		return (ERROR_INVALID);
	}

	params[1] = (ofw_word_t)args;

	for (param = 0; param < args; param++) {
		params[3 + param] = va_arg(ap, ofw_word_t);
	}

	returns = va_arg(ap, unsigned);
	if (returns > OFW_RETURNS_MAX) {
		va_end(ap);
		return (ERROR_INVALID);
	}

	params[2] = (ofw_word_t)returns;

	ret = ofw_call(params);
	if (ret == -1) {
		va_end(ap);
		return (ERROR_UNEXPECTED);
	}

	for (param = 0; param < returns; param++) {
		outp = va_arg(ap, ofw_word_t *);
		*outp = params[3 + args + param];
	}

	va_end(ap);

	return (0);
}

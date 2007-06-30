/*
 * This file is auto-generated from supervisor/functions.table.  Do not edit it directly.
 */

#include <sk/types.h>
#include <supervisor/internal.h>

static void
Exit_Stub(void *argp)
{
	struct Exit_Parameters *args = argp;

	(void)args;
	Exit();
}

static void
Install_Stub(void *argp)
{
	struct Install_Parameters *args = argp;

	(void)args;
	Install();
}

bool
supervisor_invoke(enum FunctionIndex f, void *arg, size_t argsize)
{
	switch (f) {
	case Exit_Index:
		if (sizeof (struct Exit_Parameters) != argsize)
			return (false);
		Exit_Stub(arg);
		return (true);
	case Install_Index:
		if (sizeof (struct Install_Parameters) != argsize)
			return (false);
		Install_Stub(arg);
		return (true);
	default:
		return (false);
	}
}

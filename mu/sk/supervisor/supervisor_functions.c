/*
 * This file is auto-generated from supervisor/functions.table.  Do not edit it directly.
 */

#include <supervisor/functions.h>

static void
Exit_Stub(void *argp)
{
	struct Exit_Parameters *args = argp;

	(void)args;
	Exit();
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
	default:
		return (false);
	}
}

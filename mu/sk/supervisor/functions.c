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

static void
PutString_Stub(void *argp)
{
	struct PutString_Parameters *args = argp;

	PutString(args->arg0);
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
	case PutString_Index:
		if (sizeof (struct PutString_Parameters) != argsize)
			return (false);
		PutString_Stub(arg);
		return (true);
	default:
		return (false);
	}
}

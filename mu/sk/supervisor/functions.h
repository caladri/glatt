/*
 * This file is auto-generated from supervisor/functions.table.  Do not edit it directly.
 */

#ifndef _SUPERVISOR_FUNCTIONS_H_
#define _SUPERVISOR_FUNCTIONS_H_

enum FunctionIndex {
	Exit_Index,
	PutString_Index,
};

struct Exit_Parameters {
};

struct PutString_Parameters {
	const char *arg0;
};

#endif /* !_SUPERVISOR_FUNCTIONS_H_ */

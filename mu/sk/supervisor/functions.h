/*
 * This file is auto-generated from supervisor/functions.table.  Do not edit it directly.
 */

#ifndef _SUPERVISOR_FUNCTIONS_H_
#define _SUPERVISOR_FUNCTIONS_H_

enum FunctionIndex {
	Exit_Index,
	Install_Index,
};

struct Exit_Parameters {
};
#ifdef	SUPERVISOR_FUNCTIONS
void Exit(void);
#endif /* SUPERVISOR_FUNCTIONS */

struct Install_Parameters {
	cpu_id_t	arg0;
};
#ifdef	SUPERVISOR_FUNCTIONS
void Install(cpu_id_t);
#endif /* SUPERVISOR_FUNCTIONS */

#endif /* !_SUPERVISOR_FUNCTIONS_H_ */

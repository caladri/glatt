#ifndef	_CORE_STARTUP_H_
#define	_CORE_STARTUP_H_

#include <cpu/startup.h>

enum startup_component {
	STARTUP_MP,			/* Start up multiprocessor.  */
	STARTUP_DRIVERS,		/* Register device drivers.  */
	STARTUP_DEBUGGER,		/* Enter debugger.  */
};

enum startup_order {
	STARTUP_BEFORE,
	STARTUP_FIRST,
	STARTUP_SECOND,
	STARTUP_AFTER
};

struct startup_item {
	const char *si_name;
	void (*si_function)(void *);
	void *si_arg;
	enum startup_component si_component;
	enum startup_order si_order;
};

#define	STARTUP_ITEM(name, component, order, func, arg)			\
	struct startup_item startup_ ## name = {			\
		.si_name = #name,					\
		.si_function = func,					\
		.si_arg = arg,						\
		.si_component = component,				\
		.si_order = order,					\
	};								\
	SET_ADD(startup_items, startup_ ## name)

void startup_boot(void);
void startup_main(void);

#endif /* !_CORE_STARTUP_H_ */

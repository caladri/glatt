#ifndef	_CORE_STARTUP_H_
#define	_CORE_STARTUP_H_

#include <core/queue.h>
#include <cpu/startup.h>

enum startup_component {
	STARTUP_PMAP,			/* Late-stage pmap setup.  */
	STARTUP_POOL,			/* Setup pools for later startup.  */
	STARTUP_ROOT,			/* Attach root device.  */
	STARTUP_MP,			/* Start up multiprocessor.  */
	STARTUP_DRIVERS,		/* Register device drivers.  */
	STARTUP_MAIN,			/* Enter main loop.  */
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
	TAILQ_ENTRY(struct startup_item) si_link;
};

#define	STARTUP_ITEM(name, component, order, func, arg)			\
	static struct startup_item startup_item_ ## name = {		\
		.si_name = #name,					\
		.si_function = func,					\
		.si_arg = arg,						\
		.si_component = component,				\
		.si_order = order,					\
	};								\
	SET_ADD(startup_items, startup_item_ ## name)

extern bool startup_early;	/* true if PCPU, etc., aren't usable.  */

void startup_init(void);
void startup_main(void);

#endif /* !_CORE_STARTUP_H_ */

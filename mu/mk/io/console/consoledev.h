#ifndef	_IO_DEVICE_CONSOLE_CONSOLEDEV_H_
#define	_IO_DEVICE_CONSOLE_CONSOLEDEV_H_

#include <core/spinlock.h>

struct console {
	const char *c_name;
	struct spinlock c_lock;
	void *c_softc;
	int (*c_getc)(void *, char *);
	void (*c_putc)(void *, char);
	void (*c_flush)(void *);
};

void console_init(struct console *);

#endif /* !_IO_DEVICE_CONSOLE_CONSOLEDEV_H_ */

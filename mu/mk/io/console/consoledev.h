#ifndef	_IO_DEVICE_CONSOLE_CONSOLEDEV_H_
#define	_IO_DEVICE_CONSOLE_CONSOLEDEV_H_

struct console {
	const char *c_name;
	void *c_softc;
	void (*c_putc)(void *, char);
	void (*c_flush)(void *);
};

void console_init(struct console *);

#endif /* !_IO_DEVICE_CONSOLE_CONSOLEDEV_H_ */

#ifndef	_IO_DEVICE_CONSOLE_CONSOLE_H_
#define	_IO_DEVICE_CONSOLE_CONSOLE_H_

	/* Kernel Console functions.  */

void kcputc(char);
void kcputs(const char *);
void kcprintf(const char *, ...);

#endif /* !_IO_DEVICE_CONSOLE_CONSOLE_H_ */

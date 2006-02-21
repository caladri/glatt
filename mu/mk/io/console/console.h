#ifndef	_IO_DEVICE_CONSOLE_CONSOLE_H_
#define	_IO_DEVICE_CONSOLE_CONSOLE_H_

	/* Kernel Console functions.  */

int kcgetc(char *);
void kcputc(char);
void kcputs(const char *);
void kcprintf(const char *, ...) __printf(1, 2);
void kcvprintf(const char *, va_list) __printf(1, 0);

#endif /* !_IO_DEVICE_CONSOLE_CONSOLE_H_ */

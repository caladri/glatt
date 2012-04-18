#ifndef	_IO_CONSOLE_CONSOLE_H_
#define	_IO_CONSOLE_CONSOLE_H_

	/* Kernel Console functions.  */

int kcgetc(char *);
int kcgetc_wait(char *);
void kcgetc_wakeup(bool);
void kcputc(char);
void kcputs(const char *);
void kcputsn(const char *, size_t);
void printf(const char *, ...)/* XXX need %m __printf(1, 2)*/;
void kcvprintf(const char *, va_list)/*XXX %m __printf(1, 0)*/;

#endif /* !_IO_CONSOLE_CONSOLE_H_ */

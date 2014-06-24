#ifndef	_CORE_CONSOLE_H_
#define	_CORE_CONSOLE_H_

	/* Console IPC messages.  */
#define	CONSOLE_MSG_PUTC	(0x00000001)
#define	CONSOLE_MSG_PUTS	(0x00000002)
#define	CONSOLE_MSG_GETC	(0x00000003)

#if defined(MK)
	/* Kernel Console functions.  */

int kcgetc(char *);
int kcgetc_wait(char *);
void kcgetc_wakeup(bool);
void kcputc(char);
void kcputs(const char *);
void kcputsn(const char *, size_t);
void printf(const char *, ...)/* XXX need %m __printf(1, 2)*/;
void vprintf(const char *, va_list)/*XXX %m __printf(1, 0)*/;
#endif

#endif /* !_CORE_CONSOLE_H_ */

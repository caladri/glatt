#ifndef	_SK_CONSOLE_H_
#define	_SK_CONSOLE_H_

struct sk_console {
	const char *skc_name;
	void (*skc_putchar)(void *, char);
	void *skc_private;
};

void sk_console_set(struct sk_console *);
void sk_putc(char);
void sk_puts(const char *);

#endif /* !_SK_CONSOLE_H_ */

#include <sk/types.h>
#include <sk/console.h>

static void sk_console_null_putchar(void *, char);

static struct sk_console sk_console_null = {
	.skc_name	= "null",
	.skc_putchar	= sk_console_null_putchar,
};

static struct sk_console *sk_console = &sk_console_null;

void
sk_console_set(struct sk_console *console)
{
	if (console == NULL) {
		console = &sk_console_null;
	}

	sk_puts("Switching console to: ");
	sk_puts(console->skc_name);
	sk_puts("\n");

	sk_console = console;
}

void
sk_putc(char ch)
{
	sk_console->skc_putchar(sk_console->skc_private, ch);
}

void
sk_puts(const char *s)
{
	while (*s != '\0')
		sk_putc(*s++);
}

static void
sk_console_null_putchar(void *private, char ch)
{
	(void)private;
	(void)ch;
}

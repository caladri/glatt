#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <cpu/startup.h>
#include <io/console/consoledev.h>

	/* MIO Bus.  */
#define	OCTEON_MIO_BASE			(0x0001180000000000ul)

	/* I/O routines.  */
#define	OCTEON_MIO_DEV_REG(d, reg)	((volatile uint64_t *)XKPHYS_MAP(CCA_UC, OCTEON_MIO_BASE + (d)) + (reg))
#define	OCTEON_MIO_READ(d, reg)		(*OCTEON_MIO_DEV_REG((d), (reg)))
#define	OCTEON_MIO_WRITE(d, reg, val)	(*OCTEON_MIO_DEV_REG((d), (reg)) = (val))

	/* MIO Bus Barrier.  */
#define	OCTEON_MIO_BARRIER		(0x00f8ul)

	/* MIO Bus UART.  */
#define	OCTEON_MIO_UART0_DEV_BASE	(0x0800ul)
#define	OCTEON_MIO_UART1_DEV_BASE	(0x0c00ul)

	/* Transmitter Holding.  */
#define	OCTEON_MIO_UART_REG_THR		(0x08)

	/* Line Status Register.  */
#define	OCTEON_MIO_UART_REG_LSR		(0x05)
#define	OCTEON_MIO_UART_REG_LSR_THRE	(0x20)	/* THR is Empty.  */

	/* UART Status Register.  */
#define	OCTEON_MIO_UART_REG_USR		(0x27)
#define	OCTEON_MIO_UART_REG_USR_TFNF	(0x02)	/* TX FIFO Not Full.  */

static int
octeon_console_getc(void *sc, char *chp)
{
	return (ERROR_NOT_IMPLEMENTED);
}


static void
octeon_console_putc(void *sc, char ch)
{
	/* We need \r\n.  */
	if (ch == '\n')
		octeon_console_putc(sc, '\r');

	/* Wait until the TX FIFO isn't full.  */
	for (;;) {
		uint64_t lsr, usr;
		
		lsr = OCTEON_MIO_READ(OCTEON_MIO_UART0_DEV_BASE, OCTEON_MIO_UART_REG_LSR);
		if ((lsr & OCTEON_MIO_UART_REG_LSR_THRE) != 0)
			break;

		usr = OCTEON_MIO_READ(OCTEON_MIO_UART0_DEV_BASE, OCTEON_MIO_UART_REG_USR);
		if ((usr & OCTEON_MIO_UART_REG_USR_TFNF) != 0)
			break;
	}

	OCTEON_MIO_WRITE(OCTEON_MIO_UART0_DEV_BASE, OCTEON_MIO_UART_REG_THR, (uint8_t)ch);
}

static void
octeon_console_puts(void *sc, const char *s, size_t len)
{
	while (len--)
		octeon_console_putc(sc, *s++);
}

static void
octeon_console_flush(void *sc)
{
	uint64_t val;

	val = OCTEON_MIO_READ(OCTEON_MIO_BARRIER, 0);
	(void)val;
}

static struct console octeon_console = {
	.c_name = "octeon",
	.c_softc = NULL,
	.c_getc = octeon_console_getc,
	.c_putc = octeon_console_putc,
	.c_puts = octeon_console_puts,
	.c_flush = octeon_console_flush,
};

void
platform_console_init(void)
{
	console_init(&octeon_console);
}

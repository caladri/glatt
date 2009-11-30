#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <cpu/startup.h>
#include <device/mio.h>

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

	/* Receive Buffer.  */
#define	OCTEON_MIO_UART_REG_RBR		(0x08)

	/* Transmitter Holding.  */
#define	OCTEON_MIO_UART_REG_THR		(0x08)

	/* Line Status Register.  */
#define	OCTEON_MIO_UART_REG_LSR		(0x05)
#define	OCTEON_MIO_UART_REG_LSR_DR	(0x01)	/* Data Ready.  */
#define	OCTEON_MIO_UART_REG_LSR_THRE	(0x20)	/* THR is Empty.  */

	/* UART Status Register.  */
#define	OCTEON_MIO_UART_REG_USR		(0x27)
#define	OCTEON_MIO_UART_REG_USR_TFNF	(0x02)	/* TX FIFO Not Full.  */

void
octeon_mio_bus_barrier(void)
{
	uint64_t val;

	val = OCTEON_MIO_READ(OCTEON_MIO_BARRIER, 0);
	(void)val;
}

int
octeon_mio_uart_read(unsigned unit, uint8_t *chp)
{
	uint64_t lsr, rbr;
	paddr_t base;

	switch (unit) {
	case 0:
		base = OCTEON_MIO_UART0_DEV_BASE;
		break;
	case 1:
		base = OCTEON_MIO_UART1_DEV_BASE;
		break;
	default:
		panic("%s: unsupported unit number %u", __func__, unit);
	}

	lsr = OCTEON_MIO_READ(base, OCTEON_MIO_UART_REG_LSR);
	if ((lsr & OCTEON_MIO_UART_REG_LSR_DR) == 0)
		return (ERROR_AGAIN);

	rbr = OCTEON_MIO_READ(base, OCTEON_MIO_UART_REG_RBR);
	*chp = rbr & 0xff;

	return (0);
}

void
octeon_mio_uart_write(unsigned unit, uint8_t ch)
{
	paddr_t base;

	switch (unit) {
	case 0:
		base = OCTEON_MIO_UART0_DEV_BASE;
		break;
	case 1:
		base = OCTEON_MIO_UART1_DEV_BASE;
		break;
	default:
		panic("%s: unsupported unit number %u", __func__, unit);
	}

	/* Wait until the TX FIFO isn't full.  */
	for (;;) {
		uint64_t lsr, usr;
		
		lsr = OCTEON_MIO_READ(base, OCTEON_MIO_UART_REG_LSR);
		if ((lsr & OCTEON_MIO_UART_REG_LSR_THRE) != 0)
			break;

		usr = OCTEON_MIO_READ(base, OCTEON_MIO_UART_REG_USR);
		if ((usr & OCTEON_MIO_UART_REG_USR_TFNF) != 0)
			break;
	}

	OCTEON_MIO_WRITE(base, OCTEON_MIO_UART_REG_THR, (uint8_t)ch);
}

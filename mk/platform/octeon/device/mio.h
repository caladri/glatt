#ifndef	OCTEON_MIO_H
#define	OCTEON_MIO_H

void octeon_mio_bus_barrier(void);

int octeon_mio_uart_read(unsigned, uint8_t *);
void octeon_mio_uart_write(unsigned, uint8_t);

#endif /* !OCTEON_MIO_H */

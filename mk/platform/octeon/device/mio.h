#ifndef	_DEVICE_MIO_H_
#define	_DEVICE_MIO_H_

void octeon_mio_bus_barrier(void);

int octeon_mio_uart_read(unsigned, uint8_t *);
void octeon_mio_uart_write(unsigned, uint8_t);

#endif /* !_DEVICE_MIO_H_ */

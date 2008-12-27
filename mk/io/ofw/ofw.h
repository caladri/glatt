#ifndef	_IO_OFW_OFW_H_
#define	_IO_OFW_OFW_H_

#include <io/ofw/ofw_types.h>

void ofw_init(ofw_return_t (*)(void *));

ofw_return_t ofw_call(void *);

#endif /* !_IO_OFW_OFW_H_ */

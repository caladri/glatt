#ifndef	_IO_OFW_OFW_FUNCTIONS_H_
#define	_IO_OFW_OFW_FUNCTIONS_H_

int ofw_exit(void) __noreturn;
int ofw_finddevice(const char *, ofw_package_t *);
int ofw_getprop(ofw_package_t, const char *, void *, size_t, size_t *);
int ofw_open(const char *, ofw_instance_t *);
int ofw_read(ofw_instance_t, void *, size_t, size_t *);
int ofw_write(ofw_instance_t, const void *, size_t, size_t *);

#endif /* !_IO_OFW_OFW_FUNCTIONS_H_ */

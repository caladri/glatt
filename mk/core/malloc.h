#ifndef	_CORE_MALLOC_H_
#define	_CORE_MALLOC_H_

void free(void *) __non_null(1);
void *malloc(size_t) __malloc;

#endif /* !_CORE_MALLOC_H_ */

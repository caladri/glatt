#ifndef	COMMON_H
#define	COMMON_H

#include "ipc_dispatch.h"

int vm_page_get(void **);

void exit(void);
void putchar(int);

void fatal(const char *, int);
void puts(const char *);

void printf(const char *, ...);
void vprintf(const char *, va_list);

void hexdump(const void *, size_t);

void ipc_header_print(const struct ipc_header *);
void ipc_message_print(const struct ipc_header *, const void *);

void *malloc(size_t);
void free(void *);

#endif /* !COMMON_H */

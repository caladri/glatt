#ifndef	COMMON_H
#define	COMMON_H

#include "ipc_dispatch.h"

int vm_page_get(void **);
int vm_page_free(void *);

void exit(void);
int getchar(void);
void putchar(int);
void putsn(const char *, size_t);
int getline(char *, size_t);

void fatal(const char *, int);
void puts(const char *);

void printf(const char *, ...);
void vprintf(const char *, va_list);

void hexdump(const void *, size_t);

void ipc_header_print(const struct ipc_header *);
void ipc_message_print(const struct ipc_header *, const void *);
void ipc_message_drop(const struct ipc_header *, void *);

void *malloc(size_t);
void free(void *);

int open(ipc_port_t, const char *, ipc_port_t *);
int read(ipc_port_t, void **, off_t, size_t *);
int close(ipc_port_t);
int exec(ipc_port_t);

#endif /* !COMMON_H */
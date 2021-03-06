#ifndef	COMMON_H
#define	COMMON_H

#include <ipc/ipc.h>

int vm_page_get(void **);
int vm_page_free(void *);

int vm_alloc(void **, size_t);
int vm_alloc_range(void *, void *);
int vm_free(void *, size_t);

int getchar(void);
void putchar(int);
void putsn(const char *, size_t);
void flushout(void);
int getline(char *, size_t);
int getargs(char *, size_t, unsigned *, const char **, size_t, const char *);

int splitargs(char *, unsigned *, const char **, size_t, const char *);

void fatal(const char *, int) __noreturn;
void puts(const char *);

void printf(const char *, ...);
void vprintf(const char *, va_list);

void hexdump(const void *, size_t);

void ipc_header_print(const struct ipc_header *);
void ipc_message_print(const struct ipc_header *, const void *);
void ipc_message_drop(const struct ipc_header *, void *);

void *malloc(size_t) __malloc;
void free(void *);

int open(ipc_port_t, const char *, ipc_port_t *);
int read(ipc_port_t, void **, off_t, size_t *);
int close(ipc_port_t);
int exec(ipc_port_t, ipc_port_t *, bool, unsigned, const char **);

struct fs_directory_entry;
int opendir(ipc_port_t, const char *, ipc_port_t *);
int readdir(ipc_port_t, struct fs_directory_entry *, off_t *);
int closedir(ipc_port_t);

#endif /* !COMMON_H */

#ifndef	PROCESS_H
#define	PROCESS_H

#define	PROCESS_MSG_START	(1)

extern char __progname[];

void bootstrap_main(void);
void exit(void) __noreturn;
void mu_main(void);
int process_start_data(void **, unsigned, const char **);

#endif /* !PROCESS_H */

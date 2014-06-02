#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <ipc/ipc.h>
#include <ipc/task.h>
#include <vm/vm_page.h>

#include <libmu/common.h>
#include <libmu/process.h>

void (*atexit)(void);
char __progname[128];

static struct ipc_header start_msg;

void bootstrap_main(void) __attribute__ ((__weak__));
int main(int, char *[]) __attribute__ ((__weak__));

static void process_reply(void);

void
mu_main(void)
{
	ipc_port_t task_port;
	uintptr_t *reloc;
	unsigned argc;
	char **argv;
	unsigned i;
	void *page;
	int error;

	if (bootstrap_main != NULL) {
		strlcpy(__progname, "bootstrap", sizeof __progname);

		/*
		 * The bootstrap task -- no exec args.
		 */
		bootstrap_main();
		exit();
	}

	/*
	 * Not the bootstrap task, wait for exec args.
	 */
	task_port = ipc_task_port();

	argc = 0;
	argv = NULL;
	for (;;) {
		error = ipc_port_receive(task_port, &start_msg, &page);
		if (error != 0) {
			if (error == ERROR_AGAIN) {
				error = ipc_port_wait(task_port);
				if (error != 0)
					fatal("ipc_port_wait on task port failed", error);
				continue;
			}
			fatal("ipc_port_receive on task port failed", error);
		}

		switch (start_msg.ipchdr_msg) {
		case PROCESS_MSG_START:
			break;
		case PROCESS_MSG_WAIT:
			atexit = process_reply;
			break;
		default:
			if (page != NULL) {
				error = vm_page_free(page);
				if (error != 0)
					fatal("vm_page_free failed", error);
			}
			continue;
		}

		if (page == NULL)
			break;

		argc = start_msg.ipchdr_param;
		reloc = page;
		for (i = 0; i < argc; i++) {
			if (reloc[i] > PAGE_SIZE)
				fatal("process page contains excessive relocation", ERROR_UNEXPECTED);
			reloc[i] += (uintptr_t)page;
		}
		argv = page;

		if (start_msg.ipchdr_msg == PROCESS_MSG_START)
			process_reply();

		if (argc != 0)
			strlcpy(__progname, argv[0], sizeof __progname);
		main(argc, argv);
		exit();
	}
}

int
process_start_data(void **pagep, unsigned argc, const char **argv)
{
	uintptr_t *reloc;
	const char *arg;
	unsigned i;
	char *argp;
	void *page;
	int error;

	if (argc == 0)
		fatal("process start data contains no arguments", ERROR_UNEXPECTED);

	error = vm_page_get(&page);
	if (error != 0)
		return (error);
	reloc = page;
	argp = (char *)page + (argc * sizeof argp);
	/* XXX Bounds check.  */
	for (i = 0; i < argc; i++) {
		reloc[i] = argp - (char *)page;
		arg = argv[i];
		while ((*argp++ = *arg++) != '\0')
			continue;
	}
	*pagep = page;

	return (0);
}

static void
process_reply(void)
{
	struct ipc_header ipch;
	int error;

	atexit = NULL;

	ipch = IPC_HEADER_REPLY(&start_msg);
	error = ipc_port_send(&ipch, NULL);
	if (error != 0)
		fatal("could not send start reply to parent", error);
}

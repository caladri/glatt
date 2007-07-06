#include <sk/types.h>
#include <sk/sk.h>
#include <sk/string.h>
#include <cpu/sk/memory.h>

struct cpu_exception_handler {
	char *eh_begin;
	char *eh_end;
};

static struct cpu_exception_handler GenericExceptionHandler = {
	NULL, NULL
//	GenericExceptionVector, GenericExceptionVectorEnd
};

struct cpu_exception_vector {
	paddr_t ev_vector;
	size_t ev_limit;
	struct cpu_exception_handler *ev_handler;
};

static struct cpu_exception_vector cpu_exception_vectors[] = {
	{
		0x00000180,
		0x80,
		&GenericExceptionHandler
	},
};

void
sk_cpu_supervisor_install(void)
{
	struct cpu_exception_vector *vec;
	size_t length;
	unsigned i;

	for (i = 0; i < sizeof cpu_exception_vectors /
	     sizeof cpu_exception_vectors[0]; i++) {
		vec = &cpu_exception_vectors[i];
		length = vec->ev_handler->eh_end - vec->ev_handler->eh_begin;
		if (length == 0)
			continue;
		if (length <= vec->ev_limit) {
			memcpy(XKPHYS_MAP_RAM(vec->ev_vector),
			       vec->ev_handler->eh_begin, length);
		} else {
			/* XXX panic.  */
		}
	}
}

#include <core/types.h>
#include <core/error.h>
#include <core/printf.h>

#include <libmu/common.h>

void
main(int argc, char *argv[])
{
	while (--argc) {
		const char *word = *++argv;
		printf("%s%s", word, argc == 1 ? "\n" : " ");
	}
}

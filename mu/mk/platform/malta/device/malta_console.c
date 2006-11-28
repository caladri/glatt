#include <core/types.h>
#include <core/error.h>
#include <core/startup.h>
#include <cpu/memory.h>
#include <io/device/console/console.h>
#include <io/device/console/consoledev.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	YAMON_BASE		(0x1fc00500)
#define	YAMON_PRINT_COUNT	(0x04)
#define	YAMON_GETCHAR		(0x50)

typedef	void (*malta_print_count_t)(int32_t, int32_t, int32_t);
typedef void (*malta_getchar_t)(int32_t, int32_t);

static int32_t
malta_console_map(char *p)
{
	/*
	 * This code requires page_extract() to work, which it doesn't since
	 * the addition of the vm_page structure.  Even in the immediate future
	 * page_extract will only work on virtual mappings, whereas this is
	 * probably a direct one.  Should probably go directly to pmap!
	 */
#if 0
	struct vm_page *page;
	int error;

	error = page_extract(&kernel_vm, (vaddr_t)p, &page);
	if (error != 0)
		panic("%s: page_extract failed: %m", __func__, error);
	return (KSEG0_MAP(page_address(page)));
#endif
}

static int
malta_console_getc(void *sc, char *chp)
{
	uintptr_t malta_getchar_address = YAMON_GETCHAR + (uintptr_t)sc;
	int32_t *malta_getchar_pointer = (int32_t *)malta_getchar_address;
	malta_getchar_t malta_getchar =
		(malta_getchar_t)(intptr_t)*malta_getchar_pointer;
	char ch;

	(*malta_getchar)(0, malta_console_map(&ch));
	if ((uint8_t)ch == 0xff)
		return (ERROR_AGAIN);
	switch (ch) {
	case '\r':
		*chp = '\n';
		break;
	default:
		*chp = ch;
		break;
	}
	return (0);
}

static void
malta_console_putc(void *sc, char ch)
{
	uintptr_t malta_print_count_address = YAMON_PRINT_COUNT + (uintptr_t)sc;
	int32_t *malta_print_count_pointer =
		(int32_t *)malta_print_count_address;
	malta_print_count_t malta_print_count =
		(malta_print_count_t)(intptr_t)*malta_print_count_pointer;

	(*malta_print_count)(0, malta_console_map(&ch), 1);
}

static void
malta_console_flush(void *sc)
{
	(void)sc;
}

static struct console malta_console = {
	.c_name = "malta",
	.c_softc = XKPHYS_MAP(XKPHYS_UC, YAMON_BASE),
	.c_getc = malta_console_getc,
	.c_putc = malta_console_putc,
	.c_flush = malta_console_flush,
};

void
platform_console_init(void)
{
	console_init(&malta_console);
}

#include <core/types.h>
#include <core/error.h>
#include <cpu/memory.h>
#include <cpu/pmap.h>
#include <cpu/startup.h>
#include <io/console/consoledev.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

#define	YAMON_BASE		(0x1fc00500)
#define	YAMON_PRINT_COUNT	(0x04)
#define	YAMON_GETCHAR		(0x50)

struct malta_console_softc {
	uintptr_t sc_base;
	char sc_putc_ch;
	char sc_getc_ch;
	int32_t sc_putc_addr;
	int32_t sc_getc_addr;
};

typedef	void (*malta_print_count_t)(int32_t, int32_t, int32_t);
typedef void (*malta_getchar_t)(int32_t, int32_t);

/*
 * Go straight through pmap as we're doing this before paging is really set up.
 */
static int32_t
malta_console_map(char *p)
{
	paddr_t paddr;
	vaddr_t vaddr;
	int error;

	vaddr = (vaddr_t)p;
	error = pmap_extract(&kernel_vm, PAGE_FLOOR(vaddr), &paddr);
	if (error != 0)
		panic("%s: pmap_extract failed: %m", __func__, error);
	return (KSEG0_MAP(paddr | PAGE_OFFSET(vaddr)));
}

static int
malta_console_getc(void *scp, char *chp)
{
	struct malta_console_softc *sc = scp;
	uintptr_t malta_getchar_address = YAMON_GETCHAR + sc->sc_base;
	int32_t *malta_getchar_pointer = (int32_t *)malta_getchar_address;
	malta_getchar_t malta_getchar =
		(malta_getchar_t)(intptr_t)*malta_getchar_pointer;

	(*malta_getchar)(0, sc->sc_getc_addr);
	if ((uint8_t)sc->sc_getc_ch == 0xff)
		return (ERROR_AGAIN);
	switch (sc->sc_getc_ch) {
	case '\r':
		*chp = '\n';
		break;
	case '\177':
		*chp = '\010';
		break;
	default:
		*chp = sc->sc_getc_ch;
		break;
	}
	return (0);
}

static void
malta_console_putc(void *scp, char ch)
{
	struct malta_console_softc *sc = scp;
	uintptr_t malta_print_count_address = YAMON_PRINT_COUNT + sc->sc_base;
	int32_t *malta_print_count_pointer =
		(int32_t *)malta_print_count_address;
	malta_print_count_t malta_print_count =
		(malta_print_count_t)(intptr_t)*malta_print_count_pointer;

	sc->sc_putc_ch = ch;
	(*malta_print_count)(0, sc->sc_putc_addr, 1);
}

static void
malta_console_flush(void *scp)
{
	(void)scp;
}

static struct console malta_console = {
	.c_name = "malta",
	.c_softc = NULL,
	.c_getc = malta_console_getc,
	.c_putc = malta_console_putc,
	.c_flush = malta_console_flush,
};

void
platform_console_init(void)
{
	static struct malta_console_softc softc;

	softc.sc_base = (uintptr_t)XKPHYS_MAP(XKPHYS_UC, YAMON_BASE);
	softc.sc_putc_addr = malta_console_map(&softc.sc_putc_ch);
	softc.sc_getc_addr = malta_console_map(&softc.sc_getc_ch);
	malta_console.c_softc = &softc;
	console_init(&malta_console);
}

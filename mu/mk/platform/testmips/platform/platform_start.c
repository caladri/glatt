#include <core/types.h>
#include <core/startup.h>
#include <cpu/memory.h>

void
platform_start(void)
{
	uint8_t *fb;
	uint8_t r, g, b;
	int i;

	r = g = b = 0;
	fb = XKPHYS_MAP(XKPHYS_UC, 0x12000000);
	for (i = 0; i < 640 * 480; i++) {
		fb[(i * 3) + 0] = r;
		fb[(i * 3) + 1] = g;
		fb[(i * 3) + 2] = b;
		r++; g++; b++;
	}
}

#include <core/types.h>
#include <core/startup.h>
#include <cpu/memory.h>

#define	WIDTH	640
#define	HEIGHT	20

struct bitmap {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} __attribute__ ((__packed__));

void
platform_start(void)
{
	struct bitmap *framebuffer;
	int i, j;

	framebuffer = XKPHYS_MAP(XKPHYS_UC, 0x12000000);
	for (j = 0;; j++) {
		for (i = 0; i < WIDTH * HEIGHT; i++) {
			framebuffer[i].r = (((j % 3) == 2) * i) * j;
			framebuffer[i].g = (((j % 3) == 1) * i) * j;
			framebuffer[i].b = (((j % 3) == 0) * i) * j;
		}
	}
}

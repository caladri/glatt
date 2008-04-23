#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/console/framebuffer.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	FB_BYTE_RED	0
#define	FB_BYTE_GREEN	1
#define	FB_BYTE_BLUE	2
#define	FB_BYTES	3
#define	FB_COLUMNS(fb)	((fb)->fb_width / (fb)->fb_font->f_width)
#define	FB_ROWS(fb)	((fb)->fb_height / (fb)->fb_font->f_height)

static struct rgb foreground = {
	.red = 0x00,
	.green = 0x00,
	.blue = 0x80,
};

static struct rgb background = {
	.red = 0x00,
	.green = 0x00,
	.blue = 0xff,
};

static void framebuffer_clear(struct framebuffer *);
static void framebuffer_flush(void *);
static int framebuffer_getc(void *, char *);
static void framebuffer_putc(void *, char);
static void framebuffer_putxy(struct framebuffer *, char, unsigned, unsigned);
static void framebuffer_scroll(struct framebuffer *);

void
framebuffer_init(struct framebuffer *fb, unsigned width, unsigned height)
{
	vaddr_t vaddr;
	int error;

	error = vm_alloc(&kernel_vm, width * height * FB_BYTES, &vaddr);
	if (error != 0)
		panic("%s: vm_alloc failed: %m", __func__, error);

	fb->fb_console.c_name = "framebuffer";
	fb->fb_console.c_softc = fb;
	fb->fb_console.c_getc = framebuffer_getc;
	fb->fb_console.c_putc = framebuffer_putc;
	fb->fb_console.c_flush = framebuffer_flush;

	spinlock_init(&fb->fb_lock, "framebuffer", SPINLOCK_FLAG_DEFAULT);

	spinlock_lock(&fb->fb_lock);
	fb->fb_font = &framebuffer_font_miklic_bold8x16;
	fb->fb_buffer = (uint8_t *)vaddr;

	fb->fb_width = width;
	fb->fb_height = height;
	fb->fb_column = 0;
	fb->fb_row = 0;

	framebuffer_clear(fb);

	console_init(&fb->fb_console);
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_clear(struct framebuffer *fb)
{
	unsigned x, y;

	spinlock_lock(&fb->fb_lock);
	for (x = 0; x < FB_COLUMNS(fb); x++)
		for (y = 0; y < FB_ROWS(fb); y++)
			framebuffer_putxy(fb, ' ', x, y);
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_flush(void *sc)
{
	struct framebuffer *fb;

	fb = sc;

	spinlock_lock(&fb->fb_lock);
	fb->fb_load(fb, fb->fb_buffer);
	spinlock_unlock(&fb->fb_lock);
}

static int
framebuffer_getc(void *sc, char *chp)
{
	return (ERROR_NOT_IMPLEMENTED);
}

static void
framebuffer_putc(void *sc, char ch)
{
	struct framebuffer *fb;

	fb = sc;

	spinlock_lock(&fb->fb_lock);
	if (ch == '\n') {
		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
		spinlock_unlock(&fb->fb_lock);
		return;
	}

	framebuffer_putxy(fb, ch, fb->fb_column, fb->fb_row);

	if (fb->fb_column++ == FB_COLUMNS(fb) - 1) {
		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
	}
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_putxy(struct framebuffer *fb, char ch, unsigned x, unsigned y)
{
	struct font *font;
	uint8_t *glyph;
	unsigned r, c;

	font = fb->fb_font;
	glyph = &font->f_charset[ch * fb->fb_font->f_height];

	for (r = 0; r < font->f_height; r++) {
		for (c = 0; c < font->f_width; c++) {
			struct rgb *color;
			uint8_t *pixel;
			unsigned p, s;

			p = (x * font->f_width + c) +
				((y * font->f_height + r) * fb->fb_width);
			pixel = &fb->fb_buffer[p * FB_BYTES];

			s = glyph[r] & (1 << (font->f_width - c));
			if (s == 0)
				color = &background;
			else
				color = &foreground;
			pixel[FB_BYTE_RED] = color->red;
			pixel[FB_BYTE_GREEN] = color->green;
			pixel[FB_BYTE_BLUE] = color->blue;
		}
	}
}

static void
framebuffer_scroll(struct framebuffer *fb)
{
	unsigned c;
	unsigned lh, skip;

	spinlock_lock(&fb->fb_lock);
	/*
	 * Shift up by one line-height.
	 */
	lh = fb->fb_font->f_height;
	skip = lh * fb->fb_width;
	memcpy(fb->fb_buffer, &fb->fb_buffer[skip * FB_BYTES],
	       ((fb->fb_height - lh) * fb->fb_width) * FB_BYTES);
	for (c = 0; c < FB_COLUMNS(fb); c++)
		framebuffer_putxy(fb, ' ', c, FB_ROWS(fb) - 1);
	spinlock_unlock(&fb->fb_lock);
}

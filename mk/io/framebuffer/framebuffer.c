#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/framebuffer/framebuffer.h>
#include <vm/vm.h>
#include <vm/vm_alloc.h>
#include <vm/vm_page.h>

#define	FB_BYTE_RED	0
#define	FB_BYTE_GREEN	1
#define	FB_BYTE_BLUE	2
#define	FB_BYTES	3

#define	FB_BEZCOLS	(2)
#define	FB_BEZROWS	(1)
#define	FB_BEZWIDTH(fb)		((fb)->fb_font->f_width * FB_BEZCOLS)
#define	FB_BEZHEIGHT(fb)	((fb)->fb_font->f_height * FB_BEZROWS)

#define	FB_PADCOLS	(5)
#define	FB_PADROWS	(2)
#define	FB_PADWIDTH(fb)		((fb)->fb_font->f_width * FB_PADCOLS)
#define	FB_PADHEIGHT(fb)	((fb)->fb_font->f_height * FB_PADROWS)

#define	FB_COLUMNS(fb)	(((fb)->fb_width - 2 * (FB_PADWIDTH((fb)) + FB_BEZWIDTH((fb)))) / (fb)->fb_font->f_width)
#define	FB_ROWS(fb)	(((fb)->fb_height - 2 * (FB_PADHEIGHT((fb)) + FB_BEZHEIGHT((fb)))) / (fb)->fb_font->f_height)

static struct rgb foreground = {
	.red = 0xd0,
	.green = 0xd0,
	.blue = 0xd0,
};

static void framebuffer_background(struct rgb *, unsigned, unsigned);
static void framebuffer_clear(struct framebuffer *);
static void framebuffer_flush(void *);
static int framebuffer_getc(void *, char *);
static void framebuffer_putc(void *, char);
static void framebuffer_puts(void *, const char *, size_t);
static void framebuffer_putxy(struct framebuffer *, char, unsigned, unsigned);
static void framebuffer_scroll(struct framebuffer *);

void
framebuffer_init(struct framebuffer *fb, unsigned width, unsigned height)
{
	vaddr_t vaddr;
	int error;

	error = vm_alloc(&kernel_vm, width * height * FB_BYTES, &vaddr, false);
	if (error != 0)
		panic("%s: vm_alloc failed: %m", __func__, error);

	fb->fb_console.c_name = "framebuffer";
	fb->fb_console.c_softc = fb;
	fb->fb_console.c_getc = framebuffer_getc;
	fb->fb_console.c_putc = framebuffer_putc;
	fb->fb_console.c_puts = framebuffer_puts;
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
	spinlock_unlock(&fb->fb_lock);

	console_init(&fb->fb_console);
}

static void
framebuffer_background(struct rgb *color, unsigned x, unsigned y)
{
	color->red = 0x87;
	color->green = 0x6c;
	color->blue = 0xb0;
}

static void
framebuffer_clear(struct framebuffer *fb)
{
	uint8_t *pixel;
	unsigned x, y, p;

	for (x = 0; x < fb->fb_width; x++) {
		for (y = 0; y < fb->fb_height; y++) {
			struct rgb color, scale;

			if (x < FB_PADWIDTH(fb) || x > fb->fb_width - FB_PADWIDTH(fb) ||
			    y < FB_PADHEIGHT(fb) || y > fb->fb_height - FB_PADHEIGHT(fb)) {
				/* Draw background padding.  */
				color.red = 0x40;
				color.green = 0xd6;
				color.blue = 0xff;

				scale.red = (color.red * y) / fb->fb_height;
				scale.green = (color.green * y) / fb->fb_height;
				scale.blue = ((color.blue / 4) * y) / fb->fb_height;

				color.red -= scale.red;
				color.green -= scale.green;
				color.blue -= scale.blue;
			} else if (x < FB_PADWIDTH(fb) + FB_BEZWIDTH(fb) ||
				   x > fb->fb_width - (FB_PADWIDTH(fb) + FB_BEZWIDTH(fb)) ||
				   y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) ||
				   y > fb->fb_height - (FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb))) {
				/* Draw bezel.  */
				if (x < FB_PADWIDTH(fb) + 1 || y < FB_PADHEIGHT(fb) + 1) {
					/* White highlight outside top and left.  */
					color.red = 0xff;
					color.blue = 0xff;
					color.green = 0xff;
				} else if (x > fb->fb_width - (FB_PADWIDTH(fb) + 1) ||
					   y > fb->fb_height - (FB_PADHEIGHT(fb) + 1)) {
					/* Black shadow outside bottom and right.  */
					color.red = 0x00;
					color.blue = 0x00;
					color.green = 0x00;
				} else if (x < FB_PADWIDTH(fb) + FB_BEZWIDTH(fb) - 1 ||
					   x > fb->fb_width - (FB_PADWIDTH(fb) + FB_BEZWIDTH(fb) - 1) ||
					   y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) - 1 ||
					   y > fb->fb_height - (FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) - 1)) {
					/* Gray (hint of blue) body.  */
					color.red = 0xce;
					color.blue = 0xde;
					color.green = 0xce;
				} else if (x == FB_PADWIDTH(fb) + FB_BEZWIDTH(fb) - 1 ||
					   y == FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) - 1) {
					/* Black shadow inside top and left.  */
					color.red = 0x00;
					color.blue = 0x00;
					color.green = 0x00;
				} else {
					/* White highlight inside bottom and right.  */
					color.red = 0xff;
					color.blue = 0xff;
					color.green = 0xff;
				}
			} else {
				/* Draw console background.  */
				framebuffer_background(&color, x, y);
			}

			p = x + (y * fb->fb_width);
			pixel = &fb->fb_buffer[p * FB_BYTES];

			pixel[FB_BYTE_RED] = color.red;
			pixel[FB_BYTE_GREEN] = color.green;
			pixel[FB_BYTE_BLUE] = color.blue;
		}
	}
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
framebuffer_puts(void *sc, const char *s, size_t len)
{
	struct framebuffer *fb;
	unsigned i;

	fb = sc;

	spinlock_lock(&fb->fb_lock);
	for (i = 0; i < len; i++) {
		char ch = s[i];

		if (ch == '\n') {
			if (fb->fb_row == FB_ROWS(fb) - 1)
				framebuffer_scroll(fb);
			else
				fb->fb_row++;
			fb->fb_column = 0;
			continue;
		}

		framebuffer_putxy(fb, ch, fb->fb_column, fb->fb_row);

		if (fb->fb_column++ == FB_COLUMNS(fb) - 1) {
			if (fb->fb_row == FB_ROWS(fb) - 1)
				framebuffer_scroll(fb);
			else
				fb->fb_row++;
			fb->fb_column = 0;
		}
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
			struct rgb color;
			uint8_t *pixel;
			unsigned p, s, px, py;

			px = x * font->f_width + c;
			py = y * font->f_height + r;
			px += FB_PADWIDTH(fb) + FB_BEZWIDTH(fb);
			py += FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb);
			p = px + (py * fb->fb_width);
			pixel = &fb->fb_buffer[p * FB_BYTES];

			s = glyph[r] & (1 << (font->f_width - c));
			if (s == 0)
				framebuffer_background(&color, px, py);
			else
				color = foreground;
			pixel[FB_BYTE_RED] = color.red;
			pixel[FB_BYTE_GREEN] = color.green;
			pixel[FB_BYTE_BLUE] = color.blue;
		}
	}
}

static void
framebuffer_scroll(struct framebuffer *fb)
{
	unsigned c;
	unsigned lh, skip;

	panic("%s: not yet reimplemented.", __func__);

	/*
	 * Shift up by one line-height.
	 */
	lh = fb->fb_font->f_height;
	skip = lh * fb->fb_width;
	memcpy(fb->fb_buffer, &fb->fb_buffer[skip * FB_BYTES],
	       ((fb->fb_height - lh) * fb->fb_width) * FB_BYTES);
	for (c = 0; c < FB_COLUMNS(fb); c++)
		framebuffer_putxy(fb, ' ', c, FB_ROWS(fb) - 1);
}

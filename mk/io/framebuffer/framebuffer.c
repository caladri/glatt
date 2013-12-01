#include <core/types.h>
#include <core/error.h>
#include <core/copyright.h>
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

static struct rgb background = {
	.red = 0x87,
	.green = 0x6c,
	.blue = 0xb0,
};

static void framebuffer_append(struct framebuffer *, char);
static void framebuffer_clear(struct framebuffer *, bool);
static void framebuffer_cursor(struct framebuffer *);
static void framebuffer_drawxy(struct framebuffer *, char, unsigned, unsigned, const struct rgb *, const struct rgb *);
static void framebuffer_flush(void *);
static void framebuffer_putc(void *, char);
static void framebuffer_puts(void *, const char *, size_t);
static void framebuffer_putxy(struct framebuffer *, char, unsigned, unsigned, const struct rgb *, const struct rgb *);
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
	fb->fb_console.c_getc = NULL;
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

	framebuffer_clear(fb, true);

	spinlock_unlock(&fb->fb_lock);

	console_init(&fb->fb_console);
}

static void
framebuffer_append(struct framebuffer *fb, char ch)
{
	if (ch == '\n') {
		/* Clear out possible cursor location.  */
		framebuffer_putxy(fb, ' ', fb->fb_column, fb->fb_row, NULL, &background);

		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
		return;
	}

	if (ch == '\t') {
		unsigned i;

		/* XXX Actually go to next tabstop rather than expanding a tab character in situ.  Easy enough.  */
		for (i = 0; i < 8; i++)
			framebuffer_append(fb, ' ');
		return;
	}

	framebuffer_putxy(fb, ch, fb->fb_column, fb->fb_row, &foreground, &background);

	if (fb->fb_column++ == FB_COLUMNS(fb) - 1) {
		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
	}

}

static void
framebuffer_clear(struct framebuffer *fb, bool consbox)
{
	const char *version = MK_NAME " " MK_VERSION;
	uint8_t *pixel;
	unsigned x, y, p;
	unsigned i;

	for (x = 0; x < fb->fb_width; x++) {
		for (y = 0; y < fb->fb_height; y++) {
			struct rgb color, scale;

			if (x < FB_PADWIDTH(fb) || x >= fb->fb_width - FB_PADWIDTH(fb) ||
			    y < FB_PADHEIGHT(fb) || y >= fb->fb_height - FB_PADHEIGHT(fb)) {
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
				   x >= fb->fb_width - (FB_PADWIDTH(fb) + FB_BEZWIDTH(fb)) ||
				   y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) ||
				   y >= fb->fb_height - (FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb))) {
				/* Draw bezel.  */
				if (x < FB_PADWIDTH(fb) + 1 || y < FB_PADHEIGHT(fb) + 1) {
					/* White highlight outside top and left.  */
					color.red = 0xff;
					color.blue = 0xff;
					color.green = 0xff;
				} else if (x >= fb->fb_width - (FB_PADWIDTH(fb) + 1) ||
					   y >= fb->fb_height - (FB_PADHEIGHT(fb) + 1)) {
					/* Black shadow outside bottom and right.  */
					color.red = 0x00;
					color.blue = 0x00;
					color.green = 0x00;
				} else if (x < FB_PADWIDTH(fb) + FB_BEZWIDTH(fb) - 1 ||
					   x >= fb->fb_width - (FB_PADWIDTH(fb) + FB_BEZWIDTH(fb) - 1) ||
					   y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) - 1 ||
					   y >= fb->fb_height - (FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb) - 1)) {
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
				/* Skip console background.  */
				continue;
			}

			p = x + (y * fb->fb_width);
			pixel = &fb->fb_buffer[p * FB_BYTES];

			pixel[FB_BYTE_RED] = color.red;
			pixel[FB_BYTE_GREEN] = color.green;
			pixel[FB_BYTE_BLUE] = color.blue;
		}
	}

	for (i = 0; version[i] != '\0'; i++) {
		static const struct rgb white = {
			.red = 0xff,
			.blue = 0xff,
			.green = 0xff,
		};
		static const struct rgb black = {
			.red = 0x00,
			.blue = 0x00,
			.green = 0x00,
		};
		framebuffer_drawxy(fb, version[i], i * fb->fb_font->f_width + 1, 1, &black, NULL);
		framebuffer_drawxy(fb, version[i], i * fb->fb_font->f_width, 0, &white, NULL);
	}

	if (!consbox)
		return;

	for (x = 0; x < FB_COLUMNS(fb); x++) {
		for (y = 0; y < FB_ROWS(fb); y++) {
			framebuffer_putxy(fb, ' ', x, y, NULL, &background);
		}
	}
}

static void
framebuffer_cursor(struct framebuffer *fb)
{
	static const struct rgb cursor = {
		.red = 0x00,
		.blue = 0x50,
		.green = 0xff,
	};
	framebuffer_putxy(fb, ' ', fb->fb_column, fb->fb_row, NULL, &cursor);
}

static void
framebuffer_drawxy(struct framebuffer *fb, char ch, unsigned x, unsigned y, const struct rgb *fg, const struct rgb *bg)
{
	struct font *font;
	uint8_t *glyph;
	unsigned r, c;

	font = fb->fb_font;
	glyph = &font->f_charset[ch * fb->fb_font->f_height];

	for (r = 0; r < font->f_height; r++) {
		for (c = 0; c < font->f_width; c++) {
			const struct rgb *color;
			uint8_t *pixel;
			unsigned p, s, px, py;

			px = x + c;
			py = y + r;
			p = px + (py * fb->fb_width);
			pixel = &fb->fb_buffer[p * FB_BYTES];

			s = glyph[r] & (1 << (font->f_width - c));
			if (s == 0) {
				if (bg == NULL)
					continue;
				color = bg;
			} else {
				color = fg;
			}
			pixel[FB_BYTE_RED] = color->red;
			pixel[FB_BYTE_GREEN] = color->green;
			pixel[FB_BYTE_BLUE] = color->blue;
		}
	}
}

static void
framebuffer_flush(void *sc)
{
	struct framebuffer *fb;

	fb = sc;

	spinlock_lock(&fb->fb_lock);
	framebuffer_cursor(fb);
	fb->fb_load(fb, fb->fb_buffer);
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_putc(void *sc, char ch)
{
	struct framebuffer *fb;

	fb = sc;

	spinlock_lock(&fb->fb_lock);
	framebuffer_append(fb, ch);
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
		framebuffer_append(fb, s[i]);
	}
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_putxy(struct framebuffer *fb, char ch, unsigned x, unsigned y, const struct rgb *fg, const struct rgb *bg)
{
	struct font *font;
	unsigned px, py;

	font = fb->fb_font;
	px = x * font->f_width;
	py = y * font->f_height;
	px += FB_PADWIDTH(fb) + FB_BEZWIDTH(fb);
	py += FB_PADHEIGHT(fb) + FB_BEZHEIGHT(fb);

	framebuffer_drawxy(fb, ch, px, py, fg, bg);
}

static void
framebuffer_scroll(struct framebuffer *fb)
{
	unsigned c;
	unsigned lh, skip;

	/*
	 * Shift up by one line-height.
	 */
	lh = fb->fb_font->f_height;
	skip = lh * fb->fb_width;
	memcpy(fb->fb_buffer, &fb->fb_buffer[skip * FB_BYTES],
	       ((fb->fb_height - lh) * fb->fb_width) * FB_BYTES);

	/*
	 * Redraw padding and bevel.
	 */
	framebuffer_clear(fb, false);

	/*
	 * Blank final line.
	 */
	for (c = 0; c < FB_COLUMNS(fb); c++)
		framebuffer_putxy(fb, ' ', c, FB_ROWS(fb) - 1, NULL, &background);
}

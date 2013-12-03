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

#define	FB_BEZWIDTH	(10)
#define	FB_BEZHEIGHT	(10)

#define	FB_PADROWS	(2)
#define	FB_PADHEIGHT(fb)	((fb)->fb_font->f_height * FB_PADROWS)

#define	FB_COLUMNS	80
#define	FB_ROWS(fb)	(((fb)->fb_height - 2 * (FB_PADHEIGHT((fb)) + FB_BEZHEIGHT)) / (fb)->fb_font->f_height)

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

static void framebuffer_append(struct framebuffer *, char, bool);
static void framebuffer_clear(struct framebuffer *);
static void framebuffer_cursor(struct framebuffer *);
static void framebuffer_drawxy(struct framebuffer *, char, unsigned, unsigned, const struct rgb *, const struct rgb *);
static void framebuffer_flush(void *);
static void framebuffer_putc(void *, char);
static void framebuffer_puts(void *, const char *, size_t);
static void framebuffer_putxy(struct framebuffer *, char, unsigned, unsigned, const struct rgb *, const struct rgb *, bool);
static void framebuffer_scroll(struct framebuffer *);

void
framebuffer_init(struct framebuffer *fb, unsigned width, unsigned height)
{
	unsigned x, y;
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
	fb->fb_font = &framebuffer_font_qvss8x15;
	fb->fb_buffer = (uint8_t *)vaddr;
	fb->fb_dirty_start = width * height;
	fb->fb_dirty_end = 0;

	fb->fb_width = width;
	fb->fb_height = height;
	fb->fb_column = 0;
	fb->fb_row = 0;
	fb->fb_padwidth = ((fb->fb_width - (FB_COLUMNS * fb->fb_font->f_width)) / 2) - FB_BEZWIDTH;

	error = vm_alloc(&kernel_vm, MAX(PAGE_SIZE, FB_COLUMNS * FB_ROWS(fb)), &vaddr, false);
	if (error != 0)
		panic("%s: vm_alloc failed: %m", __func__, error);
	fb->fb_text = (char *)vaddr;
	for (y = 0; y < FB_ROWS(fb); y++) {
		for (x = 0; x < FB_COLUMNS; x++) {
			framebuffer_putxy(fb, ' ', x, y, NULL, &background, true);
		}
	}

	framebuffer_clear(fb);

	spinlock_unlock(&fb->fb_lock);

	console_init(&fb->fb_console);
}

static void
framebuffer_append(struct framebuffer *fb, char ch, bool first)
{
	if (ch == '\n') {
		/* Clear out possible cursor location.  */
		framebuffer_putxy(fb, ' ', fb->fb_column, fb->fb_row, NULL, &background, true);

		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
		return;
	}

	if (ch == '\010') {
		/* Clear out possible cursor location.  */
		framebuffer_putxy(fb, ' ', fb->fb_column, fb->fb_row, NULL, &background, true);

		if (fb->fb_column != 0)
			fb->fb_column--;
		return;
	}

	if (ch == '\t') {
		unsigned i;

		/* XXX Actually go to next tabstop rather than expanding a tab character in situ.  Easy enough.  */
		for (i = 0; i < 8; i++)
			framebuffer_append(fb, ' ', i == 0 ? first : false);
		return;
	}

	framebuffer_putxy(fb, ch, fb->fb_column, fb->fb_row, &foreground, &background, first);

	if (fb->fb_column++ == FB_COLUMNS - 1) {
		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
	}
}

static void
framebuffer_clear(struct framebuffer *fb)
{
	const char *version = MK_NAME " " MK_VERSION;
	uint8_t *pixel;
	unsigned x, y, p;
	unsigned i;

	fb->fb_dirty_start = 0;
	fb->fb_dirty_end = fb->fb_width * fb->fb_height;

	for (y = 0; y < fb->fb_height; y++) {
		for (x = 0; x < fb->fb_width; x++) {
			struct rgb color, scale;

			if (x < fb->fb_padwidth || x >= fb->fb_width - fb->fb_padwidth ||
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
			} else if (x < fb->fb_padwidth + FB_BEZWIDTH ||
				   x >= fb->fb_width - (fb->fb_padwidth + FB_BEZWIDTH) ||
				   y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT ||
				   y >= fb->fb_height - (FB_PADHEIGHT(fb) + FB_BEZHEIGHT)) {
				/* Draw bezel.  */
				if (x < fb->fb_padwidth + 1 || y < FB_PADHEIGHT(fb) + 1) {
					/* White highlight outside top and left.  */
					color.red = 0xff;
					color.blue = 0xff;
					color.green = 0xff;
				} else if (x >= fb->fb_width - (fb->fb_padwidth + 1) ||
					   y >= fb->fb_height - (FB_PADHEIGHT(fb) + 1)) {
					/* Black shadow outside bottom and right.  */
					color.red = 0x00;
					color.blue = 0x00;
					color.green = 0x00;
				} else if (x < fb->fb_padwidth + FB_BEZWIDTH - 1 ||
					   x >= fb->fb_width - (fb->fb_padwidth + FB_BEZWIDTH - 1) ||
					   y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT - 1 ||
					   y >= fb->fb_height - (FB_PADHEIGHT(fb) + FB_BEZHEIGHT - 1)) {
					/* Gray (hint of blue) body.  */
					color.red = 0xce;
					color.blue = 0xde;
					color.green = 0xce;
				} else if (x == fb->fb_padwidth + FB_BEZWIDTH - 1 ||
					   y == FB_PADHEIGHT(fb) + FB_BEZHEIGHT - 1) {
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
				if (x < fb->fb_padwidth + FB_BEZWIDTH + (fb->fb_font->f_width * FB_COLUMNS) &&
				    y < FB_PADHEIGHT(fb) + FB_BEZHEIGHT + (fb->fb_font->f_height * FB_ROWS(fb))) {
					/* Skip console text background.  */
					continue;
				}
				/* Background around console box not covered by text.  */
				color = background;
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
}

static void
framebuffer_cursor(struct framebuffer *fb)
{
	static const struct rgb cursor = {
		.red = 0x00,
		.blue = 0x50,
		.green = 0xff,
	};
	framebuffer_putxy(fb, ' ', fb->fb_column, fb->fb_row, NULL, &cursor, true);
}

static void
framebuffer_drawxy(struct framebuffer *fb, char ch, unsigned x, unsigned y, const struct rgb *fg, const struct rgb *bg)
{
	const struct font *font;
	const uint8_t *glyph;
	unsigned r, c;

	if ((uint8_t)ch < fb->fb_font->f_first) {
		ch = '?';
	}

	font = fb->fb_font;
	glyph = &font->f_charset[((uint8_t)ch - fb->fb_font->f_first) * fb->fb_font->f_height];

	fb->fb_dirty_start = MIN(fb->fb_dirty_start, (y * fb->fb_width) + x);
	fb->fb_dirty_end = MAX(fb->fb_dirty_end, ((y + font->f_height) * fb->fb_width) + x + font->f_width);

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
	if (fb->fb_dirty_end > fb->fb_dirty_start) {
		fb->fb_load(fb, fb->fb_buffer, fb->fb_dirty_start * FB_BYTES, fb->fb_dirty_end * FB_BYTES);
		fb->fb_dirty_start = fb->fb_width * fb->fb_height;
		fb->fb_dirty_end = 0;
	}
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_putc(void *sc, char ch)
{
	struct framebuffer *fb;

	fb = sc;

	spinlock_lock(&fb->fb_lock);
	framebuffer_append(fb, ch, true);
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
		framebuffer_append(fb, s[i], i == 0);
	}
	spinlock_unlock(&fb->fb_lock);
}

static void
framebuffer_putxy(struct framebuffer *fb, char ch, unsigned x, unsigned y, const struct rgb *fg, const struct rgb *bg, bool force)
{
	unsigned px, py;
	char *t;

	px = x * fb->fb_font->f_width;
	py = y * fb->fb_font->f_height;
	px += fb->fb_padwidth + FB_BEZWIDTH;
	py += FB_PADHEIGHT(fb) + FB_BEZHEIGHT;

	t = &fb->fb_text[(y * FB_COLUMNS) + x];
	if (!force && *t == ch)
		return;
	*t = ch;

	framebuffer_drawxy(fb, ch, px, py, fg, bg);
}

static void
framebuffer_scroll(struct framebuffer *fb)
{
	const char *t;
	unsigned x, y;

	/*
	 * Shift up by one row.
	 */
	for (y = 1; y < FB_ROWS(fb); y++) {
		t = &fb->fb_text[y * FB_COLUMNS];
		for (x = 0; x < FB_COLUMNS; x++)
			framebuffer_putxy(fb, t[x], x, y - 1, &foreground, &background, false);
	}

	/*
	 * Blank final line.
	 */
	for (x = 0; x < FB_COLUMNS; x++)
		framebuffer_putxy(fb, ' ', x, FB_ROWS(fb) - 1, NULL, &background, false);
}

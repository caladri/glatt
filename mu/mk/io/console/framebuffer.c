#include <core/types.h>
#include <io/device/console/framebuffer.h>

#define	FB_COLUMNS(fb)	((fb)->fb_width / (fb)->fb_font->f_width)
#define	FB_ROWS(fb)	((fb)->fb_height / (fb)->fb_font->f_height)

static struct rgb foreground = { 0x00, 0x00, 0x80 };
static struct rgb background = { 0xff, 0xff, 0xff };

static struct rgb buffer[640 * 480]; /* XXX I assume an ass out of u and me.  */

static void framebuffer_flush(void *);
static void framebuffer_putc(void *, char);
static void framebuffer_putxy(struct framebuffer *, char, unsigned, unsigned);
static void framebuffer_scroll(struct framebuffer *);

void
framebuffer_init(struct framebuffer *fb, unsigned width, unsigned height)
{
	fb->fb_console.c_name = "framebuffer";
	fb->fb_console.c_softc = fb;
	fb->fb_console.c_putc = framebuffer_putc;
	fb->fb_console.c_flush = framebuffer_flush;

	fb->fb_font = &framebuffer_font_miklic_bold8x16;
	fb->fb_buffer = buffer;

	fb->fb_width = width;
	fb->fb_height = height;
	fb->fb_column = 0;
	fb->fb_row = FB_ROWS(fb) - 1;

	console_init(&fb->fb_console);
}

static void
framebuffer_flush(void *sc)
{
	struct framebuffer *fb;

	fb = sc;

	fb->fb_load(fb, fb->fb_buffer);
}

static void
framebuffer_putc(void *sc, char ch)
{
	struct framebuffer *fb;

	fb = sc;

	if (ch == '\n') {
		if (fb->fb_row == FB_ROWS(fb) - 1)
			framebuffer_scroll(fb);
		else
			fb->fb_row++;
		fb->fb_column = 0;
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
}

static void
framebuffer_putxy(struct framebuffer *fb, char ch, unsigned x, unsigned y)
{
	struct rgb *bit;
	uint8_t *character;
	unsigned r, c, s;

	character = &fb->fb_font->f_charset[ch * fb->fb_font->f_height];
	for (r = 0; r < fb->fb_font->f_height; r++) {
		for (c = 0; c < fb->fb_font->f_width; c++) {
			s = character[r] & (1 << (fb->fb_font->f_width - c));
			bit = &fb->fb_buffer[((x * fb->fb_font->f_width) + c) +
				(((y * fb->fb_font->f_height) + r) *
				 fb->fb_width)];
			if (s == 0)
				*bit = background;
			else
				*bit = foreground;
		}
	}
}

static void
framebuffer_scroll(struct framebuffer *fb)
{
	framebuffer_flush(fb);
}

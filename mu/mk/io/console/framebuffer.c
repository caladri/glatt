#include <core/types.h>
#include <core/error.h>
#include <core/string.h>
#include <io/device/console/framebuffer.h>
#include <vm/alloc.h>
#include <vm/page.h>
#include <vm/vm.h>

#define	FB_COLUMNS(fb)	((fb)->fb_width / (fb)->fb_font->f_width)
#define	FB_ROWS(fb)	((fb)->fb_height / (fb)->fb_font->f_height)

static struct bgr foreground = { 0x80, 0x00, 0x00 };
static struct bgr background = { 0xff, 0xff, 0xff };

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

	error = vm_alloc(&kernel_vm, width * height * sizeof *fb->fb_buffer,
			 &vaddr);
	if (error != 0)
		panic("%s: vm_alloc failed: %u", __func__, error);

	fb->fb_console.c_name = "framebuffer";
	fb->fb_console.c_softc = fb;
	fb->fb_console.c_getc = framebuffer_getc;
	fb->fb_console.c_putc = framebuffer_putc;
	fb->fb_console.c_flush = framebuffer_flush;

	spinlock_init(&fb->fb_lock, "framebuffer");

	spinlock_lock(&fb->fb_lock);
	fb->fb_font = &framebuffer_font_miklic_bold8x16;
	fb->fb_buffer = (struct bgr *)vaddr;

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
	struct bgr *bit;
	uint8_t *glyph;
	unsigned r, c, s;

	glyph = &fb->fb_font->f_charset[ch * fb->fb_font->f_height];
	for (r = 0; r < fb->fb_font->f_height; r++) {
		for (c = 0; c < fb->fb_font->f_width; c++) {
			s = glyph[r] & (1 << (fb->fb_font->f_width - c));
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
	unsigned c;
	unsigned lh, skip;

	spinlock_lock(&fb->fb_lock);
	/*
	 * Shift up by one line-height.
	 */
	lh = fb->fb_font->f_height;
	skip = lh * fb->fb_width;
	memcpy(fb->fb_buffer, &fb->fb_buffer[skip],
	       ((fb->fb_height - lh) * fb->fb_width) * sizeof (struct bgr));
	for (c = 0; c < FB_COLUMNS(fb); c++)
		framebuffer_putxy(fb, ' ', c, FB_ROWS(fb) - 1);
	spinlock_unlock(&fb->fb_lock);
}

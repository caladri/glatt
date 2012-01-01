#ifndef	_IO_CONSOLE_FRAMEBUFFER_H_
#define	_IO_CONSOLE_FRAMEBUFFER_H_

#include <core/spinlock.h>
#include <io/console/consoledev.h>

struct font {
	uint8_t *f_charset;	/* Charset in 8-bit bitmap format.  */
	unsigned f_width;	/* Character width.  */
	unsigned f_height;	/* Character height.  */
};

struct rgb {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

struct framebuffer {
	struct console fb_console;	/* Associated console.  */
	struct spinlock fb_lock;	/* Protects data.  */
	struct font *fb_font;		/* Current font.  */
	uint8_t *fb_buffer;
	unsigned fb_width;		/* How many pixels wide?  */
	unsigned fb_height;		/* How many pixels high?  */
	unsigned fb_column;		/* Current column.  */
	unsigned fb_row;		/* Current row.  */
	void *fb_softc;
	void (*fb_load)(struct framebuffer *, const uint8_t *);
};

extern struct font framebuffer_font_miklic_bold8x16;

void framebuffer_init(struct framebuffer *, unsigned, unsigned);

#endif /* !_IO_CONSOLE_FRAMEBUFFER_H_ */

#ifndef	_IO_FRAMEBUFFER_FRAMEBUFFER_H_
#define	_IO_FRAMEBUFFER_FRAMEBUFFER_H_

#include <core/spinlock.h>
#include <core/consoledev.h>

struct font {
	const uint8_t *f_charset;	/* Charset in 8-bit bitmap format.  */
	unsigned f_width;	/* Character width.  */
	unsigned f_height;	/* Character height.  */
	uint8_t f_first;	/* First character in font.  */
};

struct rgb {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

struct framebuffer {
	struct console fb_console;	/* Associated console.  */
	struct spinlock fb_lock;	/* Protects data.  */
	const struct font *fb_font;	/* Current font.  */
	uint8_t *fb_buffer;
	unsigned fb_width;		/* How many pixels wide?  */
	unsigned fb_height;		/* How many pixels high?  */
	unsigned fb_column;		/* Current column.  */
	unsigned fb_row;		/* Current row.  */
	void *fb_softc;
	void (*fb_load)(struct framebuffer *, const uint8_t *);
};

extern const struct font framebuffer_font_qvss8x15;

void framebuffer_init(struct framebuffer *, unsigned, unsigned);

#endif /* !_IO_FRAMEBUFFER_FRAMEBUFFER_H_ */

#ifndef	_IO_DEVICE_CONSOLE_FRAMEBUFFER_H_
#define	_IO_DEVICE_CONSOLE_FRAMEBUFFER_H_

#include <io/device/console/consoledev.h>

struct font {
	uint8_t *f_charset;	/* Charset in 8-bit bitmap format.  */
	unsigned f_width;	/* Character width.  */
	unsigned f_height;	/* Character height.  */
};

struct bgr {
	uint8_t b;
	uint8_t g;
	uint8_t r;
} __attribute__ ((__packed__));

struct framebuffer {
	struct console fb_console;	/* Associated console.  */
	struct font *fb_font;		/* Current font.  */
	struct bgr *fb_buffer;
#if 0 /* XXX assuming 24-bit.  */
	unsigned fb_bits;		/* How many bits is each pixel?  */
#endif
	unsigned fb_width;		/* How many pixels wide?  */
	unsigned fb_height;		/* How many pixels high?  */
	unsigned fb_column;		/* Current column.  */
	unsigned fb_row;		/* Current row.  */
	void *fb_softc;
	void (*fb_load)(struct framebuffer *, struct bgr *);
};

extern struct font framebuffer_font_miklic_bold8x16;

void framebuffer_init(struct framebuffer *, unsigned, unsigned);

#endif /* !_IO_DEVICE_CONSOLE_FRAMEBUFFER_H_ */

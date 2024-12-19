// berry-dm
// Copyright © 2015-2024 Yuichiro Nakada

#define TB_IMPL
#define TB_OPT_V1_COMPAT
#include "termbox.h"
#include "termbox_fire.h"

#include "aimage.h"

struct term_buf buf;

/*int w, h, frames;
int px=1, py=1, f=0;
unsigned char *screen = 0;
int cx, cy;*/

uint64_t ui_termbox_peek_event()
{
	struct tb_event ev;
	tb_peek_event(&ev, 100);

	uint32_t ch = ev.ch;
	switch (ev.key) {
	case TB_KEY_ESC:
		return 0x1b;
	case TB_KEY_BACKSPACE:
	case TB_KEY_BACKSPACE2:
		return 0x7f;
	case TB_KEY_ENTER:
		return 0x0a;
	case TB_KEY_ARROW_UP:
		return 0x415b1b;
	case TB_KEY_ARROW_DOWN:
		return 0x425b1b;
	case TB_KEY_ARROW_RIGHT:
		return 0x435b1b;
	case TB_KEY_ARROW_LEFT:
		return 0x445b1b;
	case TB_KEY_F7:
		return 0x7e38325b1b;
	case TB_KEY_F8:
		return 0x7e39325b1b;
	case TB_KEY_F9:
		return 0x7e30325b1b;
	case TB_KEY_F10:
		return 0x7e31325b1b;
	case TB_KEY_F11:
		return 0x7e33325b1b;
	case TB_KEY_F12:
		return 0x7e34325b1b;
	}

	return ch;
}

void ui_termbox_draw(uint8_t *data)
{
	int cx, cy;

	// clear
	tb_clear();

	// screen size
	int sw = tb_width();
	int sh = tb_height();
	if (sw!=buf.width || sh!=buf.height) {
		buf.width = sw;
		buf.height = sh;
	}
	cx = buf.width/2 - 11/2;
	cy = buf.height/2 - 4/*N_FIELDS*//2;

	// animation
//	fire(&buf);

	// image
	if (screen) {
		pimage(screen+f*w*h, 1, py, w, h);
		f++;
		if (f>=frames) f = 0;
	}
/*	if (conf->s[CTEXT]) {
		ptext(conf->s[CTEXT]);
	} else if (conf->s[CIMAGE]) {
		pimage(screen+f*w*h, 1, py, w, h);
		f++;
		if (f>=frames) {
			f = 0;
		}
	}*/

#define XPOS		15
#define YPOS		3
#define COLUMN		30
#define MAXCOLUMN	60

	struct tb_cell* c = tb_cell_buffer();

	for (int line=0; line < buf.height; line++) {
		for (int column=0; column < buf.width; column++) {
			if (line>=YPOS && line<=YPOS+6 && column>=XPOS && column<XPOS+COLUMN) {
				// MENU
				line -= YPOS;
				column -= XPOS;
				int n = (line+cy-2)*buf.width +column +(cx-12);
				int sel = data[0];
				c[n].ch = data[1 +line*COLUMN +column];
				if (sel*2 == line) {
					c[n].fg = 0; // selected
					c[n].bg = TB_REVERSE;
				} else {
					c[n].fg = 0;
					c[n].bg = TB_DEFAULT;
				}

				line += YPOS;
				column += XPOS;
			} else if (line>=YPOS+9 && line<=YPOS+9 && column>=XPOS && column<XPOS+COLUMN) {
				// message
				column -= XPOS;
				int n = (line+cy-YPOS-2)*buf.width +column +(cx-12);
				c[n].ch = data[1 +7*COLUMN +column];
				c[n].fg = TB_RED | TB_BOLD;
				c[n].bg = TB_DEFAULT;

				column += XPOS;
			} else if (line>=18 && line<20 && column>=0 && column<MAXCOLUMN) {
				// statusbar
				int n = buf.width*(buf.height-1) +column;
				if (line==19) n += buf.width -MAXCOLUMN;

				line -= 18;

				c[n].ch = data[1 +COLUMN*8 +MAXCOLUMN*line +column];
				c[n].fg = TB_MAGENTA | TB_BOLD;
				c[n].bg = TB_DEFAULT;

				line += 18;
			}
		}
	}

	tb_present();
}

int ui_termbox_init()
{
	tb_init();
	tb_select_output_mode(TB_OUTPUT_256);
//	tb_clear();

	buf.width = tb_width();
	buf.height = tb_height();
	fire_init(&buf);

//	atexit(tb_shutdown);
	return 0;
}

void ui_termbox_shutdown()
{
/*	if (conf->s[CIMAGE]) {
		free(screen);
	}*/
	fire_free(&buf);
	tb_shutdown();
}

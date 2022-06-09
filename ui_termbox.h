// berry-dm
// Copyright © 2015,2022 Yuichiro Nakada

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

uint64_t ui_peek_event()
{
	struct tb_event ev;
	tb_peek_event(&ev, 100);
	return ev.key;
}

void ui_draw(uint8_t *data)
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

#if 0
		if (conf->s[CIMAGE]) {
			if (screen) {
				free(screen);
			}
			screen = aviewer_init(conf->s[CIMAGE], buf.width*0.4, buf.height*0.4, &w, &h, &frames);
			if (h>0 && h<buf.height) {
				py = (buf.height-h)/2;        // center
			}
		}
#endif
	}
	cx = buf.width/2 - 11/2;
	cy = buf.height/2 - 4/*N_FIELDS*//2;

	// animation
	fire(&buf);

	// image
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

//	tb_print(1, buf.height-2, TB_MAGENTA | TB_BOLD, TB_DEFAULT, &data[1 +COLUMN*8]);

//	int XPOS = (buf.width/2)-30/2;
//	int sx = (buf.width -MAXCOLUMN)/2;

	struct tb_cell* c = tb_cell_buffer();

	for (int line=0; line < buf.height; line++) {
		for (int column=0; column < buf.width; column++) {
			if (line>=YPOS && line<=YPOS+6 && column>=XPOS && column<XPOS+COLUMN) {
				// MENU
				line -= YPOS;
				column -= XPOS;
				int n = (line+cy)*buf.width +column +(cx-12);
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
			} else if (line>=18 && line<20 && column>=0 && column<MAXCOLUMN) {
				// statusbar
				int n = buf.width*(buf.height-1) +column;
				if (line==19) n += buf.width -MAXCOLUMN;
//				int n = buf.width*line +column;
//				if (line==18) n -= sx;
//				else n += sx -buf.width;
//				if (line==19) n += -buf.width;
				line -= 18;

				c[n].ch = data[1 +COLUMN*8 +MAXCOLUMN*line +column];
				c[n].fg = TB_MAGENTA | TB_BOLD;
				c[n].bg = TB_DEFAULT;

				line += 18;
//				tb_change_cell(x, y, uni, fg, bg);
			}
		}
	}
/*	for (int n=0; n < 256; n++) {
		c[n].ch = *data++;
		c[n].fg = 0;
		c[n].bg = TB_DEFAULT;
	}*/

	tb_present();
}

int ui_init()
{
	tb_init();
	tb_select_output_mode(TB_OUTPUT_256);
//	tb_clear();

	buf.width = tb_width();
	buf.height = tb_height();
	fire_init(&buf);

//	atexit(tb_shutdown);
}

void ui_shutdown()
{
/*	if (conf->s[CIMAGE]) {
		free(screen);
	}*/
	fire_free(&buf);
	tb_shutdown();
}

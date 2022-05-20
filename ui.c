// berry-dm
// Copyright © 2015,2022 Yuichiro Nakada

// clang -o ui ui.c 3rd/ini.c -lm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define TB_IMPL
#define TB_OPT_V1_COMPAT
#include "termbox.h"
#include "termbox_fire.h"
#include <time.h>
#include "3rd/ini.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rd/stb_image.h"
#include "aimage.h"

//#define DEBUG
#ifdef DEBUG
int authenticate(char *service, char *user, char *pass)
{
	return 1;
}
#else
// login.c
extern int authenticate(char *service, char *user, char *pass);
#endif

#define CONFIG		"/etc/berry-dm.conf"

int split(char *data, char *argv[], int size)
{
	int i = 0;
	argv[i] = strtok(data, ",");
	do {
		argv[++i] = strtok(NULL, ",");
	} while ((i < size) && (argv[i] != NULL));
	return i;
}

#define CSESSIONS	0
#define CUSERS		1
#define CLANGUAGES	2
#define CIMAGE		3
#define CTEXT		4
#define CF7		5
#define CF8		6
#define CF9		7
#define CF10		8
#define CF11		9
#define CF12		10
#define CSTATUSBAR	11
#define CNUM		12
typedef struct {
	char* s[CNUM];
	int fields_count[3];
	char* fields[3][CNUM];
} configuration;

int handler(void* user, const char* section, const char* name, const char* value)
{
	configuration* pconfig = (configuration*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("config", "sessions")) {
		pconfig->s[CSESSIONS] = strdup(value);
		pconfig->fields_count[CSESSIONS] = split(pconfig->s[CSESSIONS], pconfig->fields[CSESSIONS], 10);
	} else if (MATCH("config", "users")) {
		pconfig->s[CUSERS] = strdup(value);
		pconfig->fields_count[CUSERS] = split(pconfig->s[CUSERS], pconfig->fields[CUSERS], 10);
	} else if (MATCH("config", "languages")) {
		pconfig->s[CLANGUAGES] = strdup(value);
		pconfig->fields_count[CLANGUAGES] = split(pconfig->s[CLANGUAGES], pconfig->fields[CLANGUAGES], 10);
	} else if (MATCH("config", "image")) {
		pconfig->s[CIMAGE] = strdup(value);
	} else if (MATCH("config", "text")) {
		pconfig->s[CTEXT] = strdup(value);
	} else if (MATCH("config", "F7")) {
		pconfig->s[CF7] = strdup(value);
	} else if (MATCH("config", "F8")) {
		pconfig->s[CF8] = strdup(value);
	} else if (MATCH("config", "F9")) {
		pconfig->s[CF9] = strdup(value);
	} else if (MATCH("config", "F10")) {
		pconfig->s[CF10] = strdup(value);
	} else if (MATCH("config", "F11")) {
		pconfig->s[CF11] = strdup(value);
	} else if (MATCH("config", "F12")) {
		pconfig->s[CF12] = strdup(value);
	} else if (MATCH("config", "statusbar")) {
		pconfig->s[CSTATUSBAR] = strdup(value);
	} else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}

void ptext(char *name)
{
	char buff[256];
	FILE *fp = fopen(name, "r");
	if (!fp) {
		return;
	}
	while (fgets(buff, 256, fp) != NULL) {
		printf("%s", buff);
//		tb_print(buff, 1, py, TB_MAGENTA | TB_BOLD, TB_DEFAULT);
	}
	fclose(fp);
}

static size_t width;// = 80;
static size_t height;// = 24;
void ui(configuration *conf)
{
	int field = 1;
	int sel[3];
	sel[0] = sel[1] = sel[2] = 0;

	char str[256];
	str[0] = 0;

	struct tb_event ev;
	unsigned int cur_pos = 0;
	unsigned int max_pos = 0;

	tb_init();
	tb_select_output_mode(TB_OUTPUT_256);
//	tb_clear();

	struct term_buf buf;
	buf.width = tb_width();
	buf.height = tb_height();
	fire_init(&buf);

	int w, h, frames;
	int px=1, py=1, f=0;
	unsigned char *screen = 0;
	int cx, cy;
	do {
		// clear
		tb_clear();

		// screen size
		int sw = tb_width();
		int sh = tb_height();
		if (sw!=width || sh!=height) {
//			fire_free(&buf);
/*			tb_shutdown();
			tb_init();
			tb_select_output_mode(TB_OUTPUT_256);*/
//			buf.width = tb_width();
//			buf.height = tb_height();
//			fire_init(&buf);

			width = sw;
			height = sh;

			cx = width/2 - 11/2;
			cy = height/2 - 4/*N_FIELDS*//2;

			if (conf->s[CIMAGE]) {
				if (screen) {
					free(screen);
				}
				screen = aviewer_init(conf->s[CIMAGE], width*0.4, height*0.4, &w, &h, &frames);
				if (h>0 && h<height) {
					py = (height-h)/2;        // center
				}
			}
		/*} else {
			struct tb_cell* c = tb_cell_buffer();
			for (int y=0; y<height; y++) {
				for (int x=0; x<width; x++) {
					c->ch = ' ';
//					c->fg = 0;
//					c->fg = 16 + x*216/width;
//					c->bg = 16 + y*216/height
					c->bg = 16 +1+ y*16/height *6;
//					c->fg = (x*16/width)<<4;
//					c->bg = (y*32/height)<<3;
					c++;
				}
			}*/
		}

/*		static int timer = 0;
		if (++timer>10) {
//			send_clear();
			printf("\033[0m\033[2J");
			timer = 0;
		}*/

		// animation
		fire(&buf);

		// image
		if (conf->s[CTEXT]) {
			ptext(conf->s[CTEXT]);
		} else if (conf->s[CIMAGE]) {
//			tb_print("animation!", cx-4, cy+8, TB_RED | TB_BOLD, TB_DEFAULT);
			pimage(screen+f*w*h, 1, py, w, h);
			f++;
			if (f>=frames) {
				f = 0;
			}
		}

		// function key
		if (conf->s[CSTATUSBAR]) {
			tb_print(width-strlen(conf->s[CSTATUSBAR])-1, height-2, TB_BLUE | TB_BOLD, TB_DEFAULT, conf->s[CSTATUSBAR]);
		}

		// date
		time_t now = time(NULL);
		tb_print(1, height-2, TB_MAGENTA | TB_BOLD, TB_DEFAULT, ctime(&now));

		// menu
		int bg = TB_DEFAULT;
		if (field==0) {
			bg = TB_REVERSE;
		}
		tb_printf(cx-12, cy,   0, bg, " SESSION  : %*s ", 20, conf->fields[CSESSIONS][sel[CSESSIONS]*2]);

		bg = TB_DEFAULT;
		if (field==1) {
			bg = TB_REVERSE;
		}
		tb_printf(cx-12, cy+2, 0, bg, " USER     : %*s ", 20, conf->fields[CUSERS][sel[CUSERS]]);

		tb_printf(cx-12, cy+4, 0, TB_DEFAULT, " PASSWORD : %*s ", 20, str);

		bg = TB_DEFAULT;
		if (field==2) {
			bg = TB_REVERSE;
		}
		tb_printf(cx-12, cy+6, 0, bg, " LANGUAGE : %*s ", 20, conf->fields[CLANGUAGES][sel[CLANGUAGES]*2]);

		// show
		tb_present();

		// input
		int e = tb_peek_event(&ev, 100);
		if (e == -1) {
			continue;
		}
		if (ev.type != TB_EVENT_KEY) {
			continue;
		}
#ifdef DEBUG
		if (ev.key == TB_KEY_ESC) {
			break;
		}
//		tb_printf(1, height-3, TB_MAGENTA | TB_BOLD, TB_DEFAULT, "key:%x ch:%c", ev.key, ev.ch);
//		tb_present();
#endif

//		ELOCATE(cy+4, cx+cur_pos);
		switch (ev.key) {
		case 0x1b:		// Esc
			continue;
		case TB_KEY_BACKSPACE:	// Back space 0x8
		case TB_KEY_BACKSPACE2:	// 0x7f
			if (cur_pos!=0) {
				cur_pos--;
				str[cur_pos] = 0;
			}
			break;
		case TB_KEY_ENTER:	// Enter
//			tb_print("Authenticating", cx-4, cy+8, TB_MAGENTA | TB_BOLD, TB_DEFAULT);
			if (authenticate("system-auth", conf->fields[CUSERS][sel[CUSERS]], str)) {
//				ELOCATE(cy+8, cx-4);	// (32 - 17) / 2 = 8
//				printf("\e[48;5;206;97mNot Authenticated\e[0m"); // 17 chars
				tb_print(cx-4, cy+8, TB_RED | TB_BOLD, TB_DEFAULT, "Not Authenticated");
				tb_present();

				ev.key = 0; // avoid exit

				struct timespec req;
				req.tv_sec  = 0;
				req.tv_nsec = 500000000;
				nanosleep(&req, NULL);

//				cur_pos = 0; // clear
//				str[cur_pos] = 0;
				continue;
			}
			break;
		case TB_KEY_ARROW_UP:	// Up
			field--;
			if (field<0) {
				field = 2;
			}
			break;
		case TB_KEY_ARROW_DOWN:	// Down
			field++;
			if (field>2) {
				field = 0;
			}
			break;
		case TB_KEY_ARROW_RIGHT:// Right
			sel[field]++;
			if (sel[field]>=conf->fields_count[field]) {
				sel[field] = 0;
			}
			break;
		case TB_KEY_ARROW_LEFT:	// Left
			sel[field]--;
			if (sel[field]<0) {
				sel[field] = conf->fields_count[field]-1;
			}
			break;
		case TB_KEY_F7:		// F7
			if (conf->s[CF7]) {
				system(conf->s[CF7]);
			}
			break;
		case TB_KEY_F8:		// F8
			if (conf->s[CF8]) {
				system(conf->s[CF8]);
			}
			break;
		case TB_KEY_F9:		// F9
			if (conf->s[CF9]) {
				system(conf->s[CF9]);
			}
			break;
		case TB_KEY_F10:	// F10
			if (conf->s[CF10]) {
				system(conf->s[CF10]);
			}
			break;
		case TB_KEY_F11:	// F11
			if (conf->s[CF11]) {
				system(conf->s[CF11]);
			}
			break;
		case TB_KEY_F12:	// F12
			if (conf->s[CF12]) {
				system(conf->s[CF12]);
			}
			break;
		default:
			if (cur_pos<200 && ev.ch) {
//				str[cur_pos++] = ev.key;
				str[cur_pos++] = ev.ch;
				str[cur_pos] = 0;
			}
		}

	} while (ev.key!=TB_KEY_ENTER);

	if (conf->s[CIMAGE]) {
		free(screen);
	}
	fire_free(&buf);
	tb_shutdown();

#ifndef DEBUG
	char *p = conf->fields[CSESSIONS][sel[CSESSIONS]*2+1];
	if (!strcmp(p, "/etc/X11/berryos-xsession")) {
		// for X
		setenv("DM_XSESSION", "/etc/X11/berryos-xsession", 1);
	} else {
		// for weston etc...
		setenv("DM_RUN_SESSION", "1", 1);
		setenv("DM_XSESSION", p, 1);
		setenv("DM_XINIT", p, 1);
	}
	setenv("DM_USER", conf->fields[CUSERS][sel[CUSERS]], 1);
	setenv("LANG", conf->fields[CLANGUAGES][sel[CLANGUAGES]*2+1], 1);
#endif
}

#include <sys/klog.h>
void dm_select()
{
	configuration config;
	for (int i=0; i<CNUM; i++) {
		config.s[i] = 0;
	}
	config.fields_count[CSESSIONS] = 4;
	config.fields[CSESSIONS][0] = "LXDE";
	config.fields[CSESSIONS][1] = "/etc/X11/berryos-xsession";
	config.fields[CSESSIONS][2] = "Console";
	config.fields[CSESSIONS][3] = "bash";
	config.fields_count[CUSERS] = 2;
	config.fields[CUSERS][0] = "berry";
	config.fields[CUSERS][1] = "root";
	config.fields_count[CLANGUAGES] = 4;
	config.fields[CLANGUAGES][0] = "Japanese";
	config.fields[CLANGUAGES][1] = "ja_JP.utf8";
	config.fields[CLANGUAGES][2] = "English";
	config.fields[CLANGUAGES][3] = "en_US.utf8";
	//config.s[CTEXT] = "logo.txt";

	if (ini_parse(CONFIG, handler, &config) < 0) {
		printf("Can't load '"CONFIG"'\n");
//		return 1;
	}
	config.fields_count[CSESSIONS] /= 2;
	config.fields_count[CLANGUAGES] /= 2;

	klogctl(6/*SYSLOG_ACTION_CONSOLE_OFF*/, 0, 0);
	ui(&config);
	klogctl(7/*SYSLOG_ACTION_CONSOLE_ON*/, 0, 0);

	for (int i=0; i<CNUM; i++) if (config.s[i]) free(config.s[i]);
//	return 0;
}

#ifdef DEBUG
int main()
{
	dm_select();
	return 0;
}
#endif

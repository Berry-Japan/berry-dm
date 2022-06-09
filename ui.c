// berry-dm
// Copyright © 2015,2022 Yuichiro Nakada

// clang -o ui ui.c 3rd/ini.c -lm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "3rd/ini.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rd/stb_image.h"

#define CONFIG		"/etc/berry-dm.conf"
#define GLSL		"/etc/berry-dm.glsl"
#include "ui_termbox.h"
#include "ui.h"
uint64_t (*ui_peek_event)() = ui_glsl_peek_event;
void (*ui_draw)(uint8_t *data) = ui_glsl_draw;
int (*ui_init)() = ui_glsl_init;
void (*ui_shutdown)() = ui_glsl_shutdown;

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

#define COLUMN		30
#define MAXCOLUMN	60
static char data[256*3];

static size_t width = 60;
static size_t height = 24;
void ui(configuration *conf)
{
	int field = 1;
	int sel[3];
	sel[0] = sel[1] = sel[2] = 0;

	char str[256];
	str[0] = 0;

	unsigned int cur_pos = 0;
	unsigned int max_pos = 0;

	int w, h, frames;
	int px=1, py=1, f=0;
	unsigned char *screen = 0;
	int count = 0;
	uint64_t c = 0; // key
	if (ui_init()) {
		ui_peek_event = ui_termbox_peek_event;
		ui_draw = ui_termbox_draw;
		ui_init = ui_termbox_init;
		ui_shutdown = ui_termbox_shutdown;
		ui_init();
	}
	memset(data, 32, sizeof(data));
	do {
		// image
		if (conf->s[CTEXT]) {
//			ptext(conf->s[CTEXT]);
		} else if (conf->s[CIMAGE]) {
//			pimage(screen+f*w*h, 1, py, w, h);
			f++;
			if (f>=frames) f = 0;
		}

		// message
		if (count) {
			count--;
			if (count==1) memset(data, 32, sizeof(data));
		}

		// function key
		if (conf->s[CSTATUSBAR]) {
			sprintf(&data[1 +COLUMN*8 +width], "%*s", width, conf->s[CSTATUSBAR]);
		}

		// date
		time_t now = time(NULL);
		sprintf(&data[1 +COLUMN*8], " %s", ctime(&now));

		// menu
		sprintf(data, "%d", field);
		sprintf(&data[1 +COLUMN*0], "SESSION : %*s", 20, conf->fields[CSESSIONS][sel[CSESSIONS]*2]);

		sprintf(&data[1 +COLUMN*2], "USER    : %*s", 20, conf->fields[CUSERS][sel[CUSERS]]);

		sprintf(&data[1 +COLUMN*4], "PASSWORD: %*s", 20, str);

		sprintf(&data[1 +COLUMN*6], "LANGUAGE: %*s", 20, conf->fields[CLANGUAGES][sel[CLANGUAGES]*2]);

		// input
		data[0] = field == 2 ? 3 : field;
		ui_draw((uint8_t*)data);
		c = ui_peek_event();
#ifdef DEBUG
//		printf("%lx\n", c);
		if (c == 0x1b) break;
#endif

		switch (c) {
//		case -1:
		case 0:
		case 0x1b:		// Esc
			continue;
		case 0x7f:		// Back space
			if (cur_pos!=0) {
				cur_pos--;
				str[cur_pos] = 0;
			}
			break;
		case 0x0d:		// Enter
		case 0x0a:		// Enter
			if (authenticate("system-auth", conf->fields[CUSERS][sel[CUSERS]], str)) {
//				ELOCATE(cy+8, cx-4);	// (32 - 17) / 2 = 8
//				printf("\e[48;5;206;97mNot Authenticated\e[0m"); // 17 chars
				sprintf(&data[1 +COLUMN*7], "      Not Authenticated!      ");
				count = 20;
				c = 0;
				continue;
			}
			break;
		case 0x415b1b:		// Up
			field--;
			if (field<0) field = 2;
			break;
		case 0x425b1b:		// Down
			field++;
			if (field>2) field = 0;
			break;
		case 0x435b1b:		// Right
			sel[field]++;
			if (sel[field]>=conf->fields_count[field]) sel[field] = 0;
			break;
		case 0x445b1b:		// Left
			sel[field]--;
			if (sel[field]<0) sel[field] = conf->fields_count[field]-1;
			break;
		case 0x7e38325b1b:	// F7
			if (conf->s[CF7]) system(conf->s[CF7]);
			break;
		case 0x7e39325b1b:	// F8
			if (conf->s[CF8]) system(conf->s[CF8]);
			break;
		case 0x7e30325b1b:	// F9
			if (conf->s[CF9]) system(conf->s[CF9]);
			break;
		case 0x7e31325b1b:	// F10
			if (conf->s[CF10]) system(conf->s[CF10]);
			break;
		case 0x7e33325b1b:	// F11
			if (conf->s[CF11]) system(conf->s[CF11]);
			break;
		case 0x7e34325b1b:	// F12
			if (conf->s[CF12]) system(conf->s[CF12]);
			break;
		default:
			if (cur_pos<200) {
				str[cur_pos++] = c;
				str[cur_pos] = 0;
			}
		}

	} while (/*c!=27 &&*/ c!=10/*Enter*/ && c!=0xd);
	ui_shutdown();

	if (conf->s[CIMAGE]) {
		if (screen) free(screen);
	}

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

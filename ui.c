// berry-dm
// Copyright © 2015 Yuichiro Nakada

// clang -o ui ui.c 3rd/ini.c -lm
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include "3rd/ini.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rd/stb_image.h"
#include "image.h"

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
#define CNUM		11
typedef struct
{
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
	} else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}

#define EDEFAULT()		printf("\x1b[39m\x1b[49m")
#define ECLEAR()		printf("\033[2J")
//#define ECLEAR()		printf("\033[3J")
#define ELOCATE(x, y)		printf("\033[%d;%dH", x, y)
#define ECOL(clr)		printf("\x1b[0;48;5;%um", clr)
#define EPUT(clr)		printf("\x1b[0;48;5;%um ", clr)

#define ERIGHT()		printf("\x1b[1C")
#define ELEFT()			printf("\x1b[1D")

void ptext(char *name)
{
	char buff[256];
	FILE *fp = fopen(name, "r");
	if (!fp) return;
	while (fgets(buff, 256, fp) != NULL) {
		printf("%s", buff);
	}
	fclose(fp);
}

static size_t width = 80;
static size_t height = 24;
void ui(configuration *conf)
{
	// get terminal size
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1) {
		if (0 < ws.ws_col && ws.ws_col == (size_t)ws.ws_col) {
			width = ws.ws_col;
			height = ws.ws_row;
		} 
	}

	struct termios term;
	struct termios save;
	tcgetattr(STDIN_FILENO, &term);
	//ioctl(0, TCGETA, &term);
	save = term;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	term.c_cc[VMIN] = 0;	// 0文字入力された時点で入力を受け取る
	term.c_cc[VTIME] = 1;	// 何も入力がない場合、1/10秒待つ
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	//ioctl(0, TCSETAF, &term);

	int cx = width/2 - 11/2;
	int cy = height/2 - 4/*N_FIELDS*//2;

	int field = 0;
	int sel[3];
	sel[0] = sel[1] = sel[2] = 0;

	char str[256];
	str[0] = 0;

	long long c;
	unsigned int cur_pos = 0;
	unsigned int max_pos = 0;

	ECLEAR();
	do {
		ELOCATE(1, 1);
		if (conf->s[CTEXT]) ptext(conf->s[CTEXT]);
		else if (conf->s[CIMAGE]) aviewer(conf->s[CIMAGE], width, height);

		time_t now = time(NULL);
		ELOCATE(height-1, 0);
		printf("%s", ctime(&now));

		ELOCATE(cy,   cx-11);
		printf("SESSION  : %*s", 20, conf->fields[CSESSIONS][sel[CSESSIONS]*2]);
		ELOCATE(cy+2, cx-11);
		printf("USER     : %*s", 20, conf->fields[CUSERS][sel[CUSERS]]);
		ELOCATE(cy+4, cx-11);
		printf("PASSWORD : %*s", 20, str);
		ELOCATE(cy+6, cx-11);
		printf("LANGUAGE : %*s", 20, conf->fields[CLANGUAGES][sel[CLANGUAGES]*2]);

		c = 0;
		read(0, &c, 8);

		ELOCATE(cy+4, cx+cur_pos);
		switch (c) {
		case -1:
		case 0:
		case 0x1b:		// Esc
			continue;
		case 0x7f:		// Back space
			if (cur_pos!=0) {
				cur_pos--;
				str[cur_pos] = 0;
			}
			break;
		case 0x0a:		// Enter
			if (authenticate("system-auth", conf->fields[CUSERS][sel[CUSERS]], str)) {
				ELOCATE(cy+8, cx-11);
				printf("Not Authenticated");
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
			system(conf->s[CF7]);
			break;
		case 0x7e39325b1b:	// F8
			system(conf->s[CF8]);
			break;
		case 0x7e30325b1b:	// F9
			system(conf->s[CF9]);
			break;
		case 0x7e31325b1b:	// F10
			system(conf->s[CF10]);
			break;
		case 0x7e33325b1b:	// F11
			system(conf->s[CF11]);
			break;
		case 0x7e34325b1b:	// F12
			system(conf->s[CF12]);
			break;
		default:
			if (cur_pos<200) {
				str[cur_pos++] = c;
				str[cur_pos] = 0;
			}
			//printf("*");
			/*printf("%c", c);
			max_pos++;
			cur_pos++;*/
		}
	} while (/*c!=27 &&*/ c!=10);
	ECLEAR();
	ELOCATE(1, 1);

        EDEFAULT();
	tcsetattr(STDIN_FILENO, TCSANOW, &save);
	//ioctl(0, TCSETAF, &save);

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

void dm_select()
{
	configuration config;
	for (int i=0; i<CNUM; i++) config.s[i] = 0;
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
	config.s[CTEXT] = "logo.txt";

	if (ini_parse(CONFIG, handler, &config) < 0) {
		printf("Can't load '"CONFIG"'\n");
//		return 1;
	}
	config.fields_count[CSESSIONS] /= 2;
	config.fields_count[CLANGUAGES] /= 2;

	ui(&config);

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

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

#define CONFIG		"berry-dm.conf"

int split(char *data, char *argv[], int size)
{
	int i = 0;
	argv[i] = strtok(data, ",");
	do {
		argv[++i] = strtok(NULL, ",");
	} while ((i < size) && (argv[i] != NULL));
	return i;
}
/*int split(char *str, char delim, char ***array, int *length)
{
	int count = 0;
	char *p = str;
	// Count occurance of delim in string
	while ((p=strchr(p, delim)) != NULL) {
		*p = 0;	// Null terminate the deliminator.
		p++;	// Skip past our new null
		count++;
	}

	// allocate dynamic array
	char **res = calloc(1, count * sizeof(char *));
	if (!res) return -1;

	p = str;
	for (int k=0; k<count; k++) {
		if (*p) res[k] = p;	// Copy start of string
		p = strchr(p, 0);	// Look for next null
		p++;			// Start of next string
	}

	*array = res;
	*length = count;
	return 0;
}*/

#define CSESSIONS	0
#define CUSERS		1
#define CLANGUAGES	2
#define CIMAGE		3
#define CF10		4
#define CF11		5
#define CF12		6
typedef struct
{
	int version;
	char* s[7];

	int fields_count[3];
	char* fields[3][10];

	/*int sessions_count;
	char* sessions[10];
	int languages_count;
	char* languages[10];*/
} configuration;

int handler(void* user, const char* section, const char* name, const char* value)
{
	configuration* pconfig = (configuration*)user;

	#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("protocol", "version")) {
		pconfig->version = atoi(value);
	} else if (MATCH("config", "sessions")) {
		pconfig->s[CSESSIONS] = strdup(value);
//		pconfig->sessions_count = split(pconfig->s[CSESSIONS], pconfig->sessions, 10);
		pconfig->fields_count[CSESSIONS] = split(pconfig->s[CSESSIONS], pconfig->fields[CSESSIONS], 10);
	} else if (MATCH("config", "users")) {
		pconfig->s[CUSERS] = strdup(value);
		pconfig->fields_count[CUSERS] = split(pconfig->s[CUSERS], pconfig->fields[CUSERS], 10);
	} else if (MATCH("config", "languages")) {
		pconfig->s[CLANGUAGES] = strdup(value);
//		pconfig->languages_count = split(pconfig->s[CLANGUAGES], pconfig->languages, 10);
		pconfig->fields_count[CLANGUAGES] = split(pconfig->s[CLANGUAGES], pconfig->fields[CLANGUAGES], 10);
	} else if (MATCH("config", "image")) {
		pconfig->s[CIMAGE] = strdup(value);
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

// 拡張パレット - RBGの変換テーブル
int pal2rgb[256][3] = {
	{0,0,0},      {192,0,0},    {0,192,0},    {192,192,0},  
	{64,64,192},  {192,0,192},  {0,192,192},  {192,192,192},
	{64,64,64},   {255,0,0},    {0,255,0},    {255,255,0},  
	{128,128,255},{255,0,255},  {0,255,255},  {255,255,255},
	{0,0,0},      {0,0,95},     {0,0,135},    {0,0,175},    
	{0,0,215},    {0,0,255},    {0,95,0},     {0,95,95},    
	{0,95,135},   {0,95,175},   {0,95,215},   {0,95,255},   
	{0,135,0},    {0,135,95},   {0,135,135},  {0,135,175},  
	{0,135,215},  {0,135,255},  {0,175,0},    {0,175,95},   
	{0,175,135},  {0,175,175},  {0,175,215},  {0,175,255},  
	{0,215,0},    {0,215,95},   {0,215,135},  {0,215,175},  
	{0,215,215},  {0,215,255},  {0,255,0},    {0,255,95},   
	{0,255,135},  {0,255,175},  {0,255,215},  {0,255,255},  
	{95,0,0},     {95,0,95},    {95,0,135},   {95,0,175},   
	{95,0,215},   {95,0,255},   {95,95,0},    {95,95,95},   
	{95,95,135},  {95,95,175},  {95,95,215},  {95,95,255},  
	{95,135,0},   {95,135,95},  {95,135,135}, {95,135,175}, 
	{95,135,215}, {95,135,255}, {95,175,0},   {95,175,95},  
	{95,175,135}, {95,175,175}, {95,175,215}, {95,175,255}, 
	{95,215,0},   {95,215,95},  {95,215,135}, {95,215,175}, 
	{95,215,215}, {95,215,255}, {95,255,0},   {95,255,95},  
	{95,255,135}, {95,255,175}, {95,255,215}, {95,255,255}, 
	{135,0,0},    {135,0,95},   {135,0,135},  {135,0,175},  
	{135,0,215},  {135,0,255},  {135,95,0},   {135,95,95},  
	{135,95,135}, {135,95,175}, {135,95,215}, {135,95,255}, 
	{135,135,0},  {135,135,95}, {135,135,135},{135,135,175},
	{135,135,215},{135,135,255},{135,175,0},  {135,175,95}, 
	{135,175,135},{135,175,175},{135,175,215},{135,175,255},
	{135,215,0},  {135,215,95}, {135,215,135},{135,215,175},
	{135,215,215},{135,215,255},{135,255,0},  {135,255,95}, 
	{135,255,135},{135,255,175},{135,255,215},{135,255,255},
	{175,0,0},    {175,0,95},   {175,0,135},  {175,0,175},  
	{175,0,215},  {175,0,255},  {175,95,0},   {175,95,95},  
	{175,95,135}, {175,95,175}, {175,95,215}, {175,95,255}, 
	{175,135,0},  {175,135,95}, {175,135,135},{175,135,175},
	{175,135,215},{175,135,255},{175,175,0},  {175,175,95}, 
	{175,175,135},{175,175,175},{175,175,215},{175,175,255},
	{175,215,0},  {175,215,95}, {175,215,135},{175,215,175},
	{175,215,215},{175,215,255},{175,255,0},  {175,255,95}, 
	{175,255,135},{175,255,175},{175,255,215},{175,255,255},
	{215,0,0},    {215,0,95},   {215,0,135},  {215,0,175},  
	{215,0,215},  {215,0,255},  {215,95,0},   {215,95,95},  
	{215,95,135}, {215,95,175}, {215,95,215}, {215,95,255}, 
	{215,135,0},  {215,135,95}, {215,135,135},{215,135,175},
	{215,135,215},{215,135,255},{215,175,0},  {215,175,95}, 
	{215,175,135},{215,175,175},{215,175,215},{215,175,255},
	{215,215,0},  {215,215,95}, {215,215,135},{215,215,175},
	{215,215,215},{215,215,255},{215,255,0},  {215,255,95}, 
	{215,255,135},{215,255,175},{215,255,215},{215,255,255},
	{255,0,0},    {255,0,95},   {255,0,135},  {255,0,175},  
	{255,0,215},  {255,0,255},  {255,95,0},   {255,95,95},  
	{255,95,135}, {255,95,175}, {255,95,215}, {255,95,255}, 
	{255,135,0},  {255,135,95}, {255,135,135},{255,135,175},
	{255,135,215},{255,135,255},{255,175,0},  {255,175,95}, 
	{255,175,135},{255,175,175},{255,175,215},{255,175,255},
	{255,215,0},  {255,215,95}, {255,215,135},{255,215,175},
	{255,215,215},{255,215,255},{255,255,0},  {255,255,95}, 
	{255,255,135},{255,255,175},{255,255,215},{255,255,255},
	{8,8,8},      {18,18,18},   {28,28,28},   {38,38,38},   
	{48,48,48},   {58,58,58},   {68,68,68},   {78,78,78},   
	{88,88,88},   {98,98,98},   {108,108,108},{118,118,118},
	{128,128,128},{138,138,138},{148,148,148},{158,158,158},
	{168,168,168},{178,178,178},{188,188,188},{198,198,198},
	{208,208,208},{218,218,218},{228,228,228},{238,238,238},
};

// 三次空間から距離を得る
int distance(int r0, int g0, int b0, int r1, int g1, int b1)
{
	int r, g, b;
	r = abs(r0 - r1);
	g = abs(g0 - g1);
	b = abs(b0 - b1);
	return sqrt(r * r + g * g + b * b);
}

// RGB から 拡張カラーへの近似色を探す
int near(int r0, int g0, int b0)
{
	int dmin = 1000; // > 441.672
	int d[256] = {0};
	for (int i=0; i<256; i++) {
		d[i] = distance(r0, g0, b0, pal2rgb[i][0], pal2rgb[i][1], pal2rgb[i][2]);
		if (dmin > d[i]) dmin = d[i];
	}
	for (int i=0; i<256; i++) {
		if (d[i] == dmin) return i;
	}
	return 255;
}

void putImage(char *name)
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
//		printf("terminal_width  = %d\n", ws.ws_col);
//		printf("terminal_height = %d\n", ws.ws_row);
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

	long long c;
	unsigned int cur_pos = 0;
	unsigned int max_pos = 0;
	//char *pmp_str = "command > ";
	ECLEAR();
	do {
		ELOCATE(1, 1);
		putImage(conf->s[CIMAGE]);

		time_t now = time(NULL);
		ELOCATE(height-1, 0);
		printf("%s", ctime(&now));

		ELOCATE(cy,   cx-11);
//		printf("SESSION  : %*s", 20, conf->sessions[sel[0]*2]);
		printf("SESSION  : %*s", 20, conf->fields[CSESSIONS][sel[CSESSIONS]*2]);
		ELOCATE(cy+2, cx-11);
		printf("USER     : %*s", 20, conf->fields[CUSERS][sel[CUSERS]]);
		ELOCATE(cy+4, cx-11);
		printf("PASSWORD : ");
		ELOCATE(cy+6, cx-11);
//		printf("LANGUAGE : %*s", 20, conf->languages[sel[2]*2]);
		printf("LANGUAGE : %*s", 20, conf->fields[CLANGUAGES][sel[CLANGUAGES]*2]);

		c = 0;
		read(0, &c, 8);
//		ELOCATE(height-1, width-1);

//		ELOCATE(height-2, 1+cur_pos);
		ELOCATE(cy+4, cx+cur_pos);
		switch (c) {
		case -1:
		case 0:
		case 0x1b:		// Esc
			continue;
		case 0x08:		// Back space
			cur_pos--;
			if (cur_pos<0) cur_pos = 0;
			/*if (cur_pos > strlen(pmp_str)) {
				printf("%c \b", c);
				cur_pos--;
			}*/
			break;
		/*case 0x0a:		// Enter
			printf("\n%s", pmp_str);
			cur_pos = strlen(pmp_str);
			max_pos = cur_pos;
			break;*/
		case 0x415b1b:		// Up
			field--;
			if (field<0) field = 2;
			break;
		case 0x425b1b:		// Down
			field++;
			if (field>2) field = 0;
			break;
		case 0x435b1b:		// Right
//			sel[0]++;
//			if (sel[0]>=conf->sessions_count) sel[0] = 0;
			sel[field]++;
			if (sel[field]>=conf->fields_count[field]) sel[field] = 0;
			/*if (max_pos > cur_pos) {
				ERIGHT();
				cur_pos++;
			}*/
			break;
		case 0x445b1b:		// Left
//			sel[0]--;
//			if (sel[0]<0) sel[0] = conf->sessions_count-1;
			sel[field]--;
			if (sel[field]<0) sel[field] = conf->fields_count[field]-1;
			/*if (cur_pos > strlen(pmp_str)) {
				ELEFT();
				cur_pos--;
			}*/
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
			printf("%c", c);
			max_pos++;
			cur_pos++;
		}
	} while (c!=27 && c!=10);
	ECLEAR();
	ELOCATE(1, 1);

/*	for (int i=0; i<256; i++) EPUT(i);

	ELOCATE(7, 8);
	ECOL(near(0, 100, 0));
	printf("Hello!\n");	// メッセージを出力
	printf("Press [Enter]...\n");	// メッセージを出力
	getchar();			// キー入力を待つ*/

        EDEFAULT();
	tcsetattr(STDIN_FILENO, TCSANOW, &save);
	//ioctl(0, TCSETAF, &save);
}

int main()
{
	configuration config;
	config.s[CSESSIONS] = 0;
//	config.s[CSESSIONS] = "LXDE,/etc/X11/berryos-xsession,Console,bash";
	/*config.sessions_count = 4;
	config.sessions[0] = "LXDE";
	config.sessions[1] = "/etc/X11/berryos-xsession";
	config.sessions[2] = "Console";
	config.sessions[3] = "bash";*/
	config.fields_count[CSESSIONS] = 4;
	config.fields[CSESSIONS][0] = "LXDE";
	config.fields[CSESSIONS][1] = "/etc/X11/berryos-xsession";
	config.fields[CSESSIONS][2] = "Console";
	config.fields[CSESSIONS][3] = "bash";
	config.fields_count[CUSERS] = 2;
	config.fields[CUSERS][0] = "berry";
	config.fields[CUSERS][1] = "root";
//	config.s[CLANGUAGES] = "Japanese,ja_JP.utf8,English,en_US.utf8";
	/*config.languages_count = 4;
	config.languages[0] = "Japanese";
	config.languages[1] = "ja_JP.utf8";
	config.languages[2] = "English";
	config.languages[3] = "en_US.utf8";*/
	config.fields_count[CLANGUAGES] = 4;
	config.fields[CLANGUAGES][0] = "Japanese";
	config.fields[CLANGUAGES][1] = "ja_JP.utf8";
	config.fields[CLANGUAGES][2] = "English";
	config.fields[CLANGUAGES][3] = "en_US.utf8";
	config.s[CIMAGE] = "logo.txt";

	if (ini_parse(CONFIG, handler, &config) < 0) {
		printf("Can't load '"CONFIG"'\n");
//		return 1;
	}
//	config.sessions_count /= 2;
//	config.languages_count /= 2;
	config.fields_count[CSESSIONS] /= 2;
	config.fields_count[CLANGUAGES] /= 2;

	ui(&config);

	if (config.s[CSESSIONS]) for (int i=0; i<6; i++) free(config.s[i]);
	return 0;
}

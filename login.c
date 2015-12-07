// berry-dm
// Copyright © 2014-2015 Yuichiro Nakada
//
// gcc -o berry-dm -Os berry-dm.c login.c -lcursesw -lform -lpam -lpam_misc

#include <locale.h>
#include <form.h>
#include <time.h>
#include <string.h>

#define WIDTH 25
#define N_FIELDS	4+1


#if 0
#include <fcntl.h>
#include <linux/vt.h>
void switch_to_vt(int vt)
{
	static const char *ttys[] = {
		"/dev/tty",
		"/dev/tty0",
		"/proc/self/fd/0",
		"/dev/console",
		NULL
	};
	int i;

	for (i=0; ttys[i] != NULL; i++) {
		int fd = open(ttys[i], O_RDWR);
		if (fd < 0) continue;

		if (!isatty(fd)) goto next_iter;

		if (ioctl(fd, VT_ACTIVATE, vt)) {
			perror("vt activation failed");
			goto next_iter;
		}
		if (ioctl(fd, VT_WAITACTIVE, vt)) {
			perror("wait for vt activation failed");
			goto next_iter;
		}
		close(fd);
		break;

		next_iter:
		close(fd);
	}
}
#endif

#include <malloc.h>
#include <security/pam_appl.h>
struct pam_response *reply;

int null_conv(int num_msg, const struct pam_message **msg, struct pam_response **resp, void *appdata_ptr)
{
	*resp = reply;
	return PAM_SUCCESS;
}

static struct pam_conv conv = { null_conv, NULL };

int authenticate(char *service, char *user, char *pass)
{
	pam_handle_t *pamh = NULL;
	int retval = pam_start(service, user, &conv, &pamh);

	if (retval == PAM_SUCCESS) {
		reply = (struct pam_response *)malloc(sizeof(struct pam_response));
		reply[0].resp = pass;
		reply[0].resp_retcode = 0;

		retval = pam_authenticate(pamh, 0);

		/*if (retval == PAM_SUCCESS) {
			fprintf(stdout, "Authenticated\n");
		} else {
			fprintf(stdout, "Not Authenticated\n");
		}*/

		pam_end(pamh, PAM_SUCCESS);

		//return ( retval == PAM_SUCCESS ? 0:1 );
	}

	return ( retval == PAM_SUCCESS ? 0:1 );
}

#include <sys/sysinfo.h>
long get_uptime()
{
	struct sysinfo s_info;
	int error;
	error = sysinfo(&s_info);
	/*if (error != 0) {
		printf("code error = %d\n", error);
	}*/
	return s_info.uptime;
}

char* rtrim(char* p)
{
	int s = (strlen(p) - 1 - 1);
	for (; s > 0 && p[s] == 0x20; s--) ;
	p[s + 1] = 0;
	return p;
}

/*void set_printk(int level)
{
	FILE *fp = fopen("/proc/sys/kernel/printk", "w");
	if (fp) {
		//fprintf(fp, "0 4 1 7\n");
		//fprintf(fp, "%d 4 1 7\n", level);
		fprintf(fp, "%d\n", level);
		fclose(fp);
	}
}*/

//#include "mya.h"
#include <linux/reboot.h>
void dm_select()
{
	FIELD *field[N_FIELDS];
	FORM  *my_form;
	int ch, i;

	//set_printk(0);

	/* Initialize curses */
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	/* Initialize the fields */
	int w, h, STARTX, STARTY;
	getmaxyx(stdscr, h, w);
	STARTX = w/2 - 11/2;
	STARTY = h/2 - N_FIELDS/2;
	for (i=0; i < N_FIELDS-1; i++) {
		field[i] = new_field(1, WIDTH, STARTY + i * 2, STARTX, 0, 0);
	}
	field[N_FIELDS - 1] = NULL;

	// Set field options
	field_opts_off(field[0], O_EDIT);		// This field is a static label
//	field_opts_off(field[1], O_AUTOSKIP); /* To avoid entering the same field */
	field_opts_off(field[2], O_PUBLIC);	// This filed is like a password field
	field_opts_off(field[3], O_EDIT);		// This field is a static label
	/* after last character is entered */

	/* Create the form and post it */
	my_form = new_form(field);
	post_form(my_form);
	refresh();

	/*WINDOW *win = newwin(10, 10, h-10, w-10);
	set_form_win(my_form, win);
	box(win, ACS_VLINE, ACS_HLINE);
	wrefresh(win);*/

	int sel[2];
	sel[0] = sel[1] = 0;
	const char *array[][2] = {
		"Maynard (Weston)",
		"maynard",
		"LXDE",
		"/etc/X11/berryos-xsession",
		"Console",
		"bash",
		"Reboot",
		"reboot",
		"Shutdown",
		"shutdown -h now"
	};
	const char *lang[][2] = {
		"Japanese", "ja_JP.utf8",
		"English", "en_US.utf8",
		"Chinese", "zh_TW.utf8"
	};

//	usleep(500000);
#if 1
//	set_field_just(field[0], JUSTIFY_CENTER); /* Center Justification */
	set_field_buffer(field[0], 0, array[sel[0]][0]);
	set_field_buffer(field[1], 0, "berry");
	set_field_buffer(field[3], 0, lang[sel[1]][0]);
	/*set_field_pad(field[0], 0);
	set_field_pad(field[1], 0);
	set_field_pad(field[2], 0);
	set_field_pad(field[3], 0);*/

	/* Initialize the field  */
	mvprintw(STARTY,   STARTX-11, "SESSION  : ");
	mvprintw(STARTY+2, STARTX-11, "USER     : ");
	mvprintw(STARTY+4, STARTX-11, "PASSWORD : ");
	mvprintw(STARTY+6, STARTX-11, "LANGUAGE : ");
	set_current_field(my_form, field[2]);
	move(STARTY+4, STARTX);
	refresh();
#endif

	/* Loop through to get user requests */
	timeout(5000);
	do {
		int x, y;
//		char str[256];
		getyx(stdscr, y, x);
		//mvprintw(h-23, w-50, "%s", aa);
/*		clear();
//			form_driver(my_form, REQ_VALIDATION);
//		strcpy(str, field_buffer(field[1], 0));
//			mvprintw(0, 0, "[%s]", str);
		set_field_buffer(field[0], 0, array[sel[0]][0]);
//		set_field_buffer(field[1], 0, rtrim(str));
//		set_field_buffer(field[1], 0, field_buffer(field[1], 0));
		set_field_buffer(field[3], 0, lang[sel[1]][0]);
		mvprintw(STARTY,   STARTX-11, "SESSION  : ");
		mvprintw(STARTY+2, STARTX-11, "USER     : ");
		mvprintw(STARTY+4, STARTX-11, "PASSWORD : ");
		mvprintw(STARTY+6, STARTX-11, "LANGUAGE : ");*/
		time_t now = time(NULL);
		mvprintw(h-1, 0, "%s", ctime(&now));
		refresh();
		move(y, x);

		ch = getch();
		switch (ch) {
		case ERR: // Timeout
			break;

		case KEY_F(10):
			system("ntpd -q -p pool.ntp.org");
			break;
		case KEY_F(11):
			system("reboot");
			//sync();
			//reboot(LINUX_REBOOT_CMD_RESTART);
			break;
		case KEY_F(12):
			system("shutdown -h now");
			//sync();
			//reboot(LINUX_REBOOT_CMD_POWER_OFF);
			break;

		case 9:	// tab key
		case 10:	// enter key
		case KEY_DOWN:
			form_driver(my_form, REQ_NEXT_FIELD);
			form_driver(my_form, REQ_END_LINE);
			break;
		case KEY_BTAB:
		case KEY_UP:
			form_driver(my_form, REQ_PREV_FIELD);
			form_driver(my_form, REQ_END_LINE);
			break;
		case KEY_LEFT:
			if (field_index(current_field(my_form))!=3) {
				// Session
				sel[0]--;
				if (sel[0]<0) sel[0] = 4;
				set_field_buffer(field[0], 0, array[sel[0]][0]);
			} else {
				sel[1]--;
				if (sel[1]<0) sel[1] = 2;
				set_field_buffer(field[3], 0, lang[sel[1]][0]);
			}
			break;
		case KEY_RIGHT:
			if (field_index(current_field(my_form))!=3) {
				// Session
				sel[0]++;
				if (sel[0]>=5) sel[0] = 0;
				set_field_buffer(field[0], 0, array[sel[0]][0]);
			} else {
				sel[1]++;
				if (sel[1]>=3) sel[1] = 0;
				set_field_buffer(field[3], 0, lang[sel[1]][0]);
			}
			break;
		case KEY_BACKSPACE:
		case 127:
			form_driver(my_form, REQ_LEFT_CHAR);
		case KEY_DC:
			form_driver(my_form, REQ_DEL_CHAR);
			break;
		/*case KEY_IC:	// Insert
			if (insert_mode == INSERT) {
				insert_mode = OVERLAY;
				form_driver(my_form, REQ_OVL_MODE);
			} else {
				insert_mode = INSERT;
				form_driver(my_form, REQ_INS_MODE);
			}
			break;*/
		default:
			// If this is a normal character, it gets Printed
			form_driver(my_form, ch);
		}

		if (ch == /*KEY_F(1)*/10) {
			char *user = rtrim(strdup(field_buffer(field[1], 0)));
			char *pass = rtrim(strdup(field_buffer(field[2], 0)));
			//mvprintw(1, 0, "[%s]", user);
			//mvprintw(2, 0, "[%s]", pass);
			int r = authenticate("system-auth", user, pass);
			free(user);
			//free(pass);
			if (!r) break;
//			mvprintw(h-1, w-17, "Not Authenticated");
			mvprintw(0, 0, "Not Authenticated");
		}
	} while (1);

	// Set the field!!
	form_driver(my_form, REQ_NEXT_FIELD);

	// Get user selection
//	char *p = rtrim(field_buffer(field[0], 0));
//	if (!strcmp(p, "startlxde")) {
	const char *p = array[sel[0]][1];
//	if (!strcmp(p, "/etc/lxdm/Xsession")) {
	if (!strcmp(p, "/etc/X11/berryos-xsession")) {
		// for X
//		setenv("DM_XSESSION", "/etc/lxdm/Xsession", 1);
		setenv("DM_XSESSION", "/etc/X11/berryos-xsession", 1);
	} else {
		// for weston etc...
		setenv("DM_RUN_SESSION", "1", 1);
		setenv("DM_XSESSION", p, 1);
		setenv("DM_XINIT", p, 1);
	}
	//setenv("DM_INIT", (field_buffer(field[0], 0)), 1);
	setenv("DM_USER", rtrim(field_buffer(field[1], 0)), 1);

//	setenv("LANG", "ja_JP.utf8", 1);
	setenv("LANG", lang[sel[1]][1], 1);

	// Un post form and free the memory
	unpost_form(my_form);
	free_form(my_form);
	for (i=0; i < N_FIELDS-1; i++) {
		free_field(field[i]);
	}

	echo();
	nocbreak();
	endwin();
	//set_printk(7);

	//printf("[%s] [%s] [%s]\n", getenv("DM_XSESSION"), getenv("DM_INIT"), getenv("DM_USER"));
	return;
}

int main(int argc, char* argv[])
{
	// /bin/sh -c 'exec xinit berry-dm -- vt7'
	if (!getenv("DM_RUN_SESSION")) {
		// Check 'autologin'
		FILE *fp;
		char buff[256];
		fp = fopen("/proc/cmdline", "r");
		if (fp!=NULL) {
			fread(buff, 256, 256, fp);
			fclose(fp);
		}
//		struct timespec t;
//		clock_gettime(CLOCK_MONOTONIC, &t);
		if (strstr(buff, "autologin") && get_uptime()<30/*!getenv("DM_INIT")*/) {
//			setenv("DM_INIT", "1", 1);

			setenv("DM_USER", "berry", 1);

//			setenv("DM_XSESSION", "startlxde", 1);
//			setenv("DM_XSESSION", "/etc/lxdm/Xsession", 1);
			char *p;
			if (p=strstr(buff, "desktop=maynard")) {
				setenv("DM_RUN_SESSION", "1", 1);
				setenv("DM_XSESSION", "maynard", 1);
				setenv("DM_XINIT", "maynard", 1);
			} else {
				setenv("DM_XSESSION", "/etc/X11/berryos-xsession", 1);
			}

			if (strstr(buff, "lang=us")) setenv("LANG", "en_US.utf8", 1);
			else setenv("LANG", "ja_JP.utf8", 1);
		} else {
			//switch_to_vt(8);
			dm_select();
		}
	//} else {
		//setenv("DM_USER", "berry", 1);
		//setenv("DM_XSESSION", "startlxde", 1);
	}

	dm_main(argc, argv);

	return 0;
}

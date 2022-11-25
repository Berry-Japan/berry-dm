// berry-dm
// Copyright © 2014-2015 Yuichiro Nakada
//
// clang -o berry-dm -Os berry-dm.c login.c ui.c 3rd/ini.c -lm -lpam -lpam_misc
// clang -o berry-dm -Os berry-dm.c login.c ui.c -lcursesw -lform -lpam -lpam_misc

#include <stdlib.h>
#include <string.h>

extern void dm_select();			// ui.c
extern int dm_main(int argc, char **argv);	// berry-dm.c

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
		reply[0].resp = strdup(pass);	// ?? need strdup
		reply[0].resp_retcode = 0;

		retval = pam_authenticate(pamh, 0);

		pam_end(pamh, PAM_SUCCESS);
	}

	return ( retval == PAM_SUCCESS ? 0:1 );
}

#include <sys/sysinfo.h>
long get_uptime()
{
	struct sysinfo s_info;
	s_info.uptime = 31;	// avoid error
	sysinfo(&s_info);
	return s_info.uptime;
}

void set_printk(int level)
{
	FILE *fp = fopen("/proc/sys/kernel/printk", "w");
	if (fp) {
		//fprintf(fp, "0 4 1 7\n");
		//fprintf(fp, "%d 4 1 7\n", level);
		fprintf(fp, "%d\n", level);
		fclose(fp);
	}
}

int main(int argc, char* argv[])
{
	// /bin/sh -c 'exec xinit berry-dm -- vt7'
	if (!getenv("DM_RUN_SESSION")) {
		// Check 'autologin'
		char buff[256];
		FILE *fp = fopen("/proc/cmdline", "r");
		if (fp!=NULL) {
			fread(buff, sizeof(buff), sizeof(char), fp);
			fclose(fp);
			buff[255] = 0;
		}

		if (strstr(buff, "quiet")) set_printk(0);

//		struct timespec t;
//		clock_gettime(CLOCK_MONOTONIC, &t);
		if (strstr(buff, "autologin") && get_uptime()<30/*!getenv("DM_INIT")*/) {
//			setenv("DM_INIT", "1", 1);
			setenv("DM_USER", "berry", 1);

//			setenv("DM_XSESSION", "startlxde", 1);
//			setenv("DM_XSESSION", "/etc/lxdm/Xsession", 1);
			char *p;
			if ((p=strstr(buff, "desktop=wayfire"))) {
				setenv("DM_RUN_SESSION", "1", 1);
				setenv("DM_XSESSION", "wayfire", 1);
				setenv("DM_XINIT", "wayfire", 1);
			} else {
				setenv("DM_XSESSION", "/etc/X11/berryos-xsession", 1);
			}

			if (strstr(buff, "lang=us")) setenv("LANG", "en_US.utf8", 1);
			else setenv("LANG", "ja_JP.utf8", 1);
		} else {
			//switch_to_vt(8);
			dm_select();
		}
	}

	dm_main(argc, argv);

	return 0;
}

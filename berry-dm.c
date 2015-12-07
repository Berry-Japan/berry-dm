// gcc -Os -o berry-dm berry-dm.c -lpam -lpam_misc
/*
 * Copyright 1989 - 1994, Julianne Frances Haugh
 * Copyright 2009, Enrico Zini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Julianne F. Haugh nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JULIE HAUGH AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL JULIE HAUGH OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Some parts substantially derived from an ancestor of: */
/* su for GNU.  Run a shell with substitute user and group IDs.
   Copyright (C) 1992-2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#define VERSION "0.1"
#define NAME "berry-dm"
#ifndef DM_SESSION
#define DM_SESSION "/usr/sbin/berry-dm"
#endif
/* #define DEBUG_DM */


//#include "config.h"
#include <getopt.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/vt.h>

#include <security/pam_appl.h>
#include <security/pam_misc.h>

static struct pam_conv conv = {
	misc_conv,
	NULL
};

/* compatibility with different versions of Linux-PAM */
#ifndef PAM_ESTABLISH_CRED
#define PAM_ESTABLISH_CRED PAM_CRED_ESTABLISH
#endif
#ifndef PAM_DELETE_CRED
#define PAM_DELETE_CRED PAM_CRED_DELETE
#endif

#define _(...) (__VA_ARGS__)

/* Copy string pointed by B to array A with size checking.  It was originally
   in lmain.c but is _very_ useful elsewhere.  Some setuid root programs with
   very sloppy coding used to assume that BUFSIZ will always be enough...  */
/* danger - side effects */
#define STRFCPY(A,B) \
        (strncpy((A), (B), sizeof(A) - 1), (A)[sizeof(A) - 1] = '\0')

/*
 * Exit codes used by shadow programs
 */
#define E_SUCCESS               0       /* success */
#define E_NOPERM                1       /* permission denied */
#define E_USAGE                 2       /* invalid command syntax */
#define E_BAD_ARG               3       /* invalid argument to option */
#define E_PASSWD_NOTFOUND       14      /* not found password file */
#define E_SHADOW_NOTFOUND       15      /* not found shadow password file */
#define E_GROUP_NOTFOUND        16      /* not found group file */
#define E_GSHADOW_NOTFOUND      17      /* not found shadow group file */
#define E_CMD_NOEXEC            126     /* can't run command/shell */
#define E_CMD_NOTFOUND          127     /* can't find command/shell to run */

#ifdef DEBUG_DM
/* Turn syslog into fprintf, for debugging */
#define syslog(prio, ...) do { fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while (0)
#endif

/*
 * Assorted #defines to control su's behavior
 */
/*
 * Global variables
 */
/* User we are changing to */
static char name[BUFSIZ];

static pam_handle_t *pamh = NULL;
static int caught = 0;

/* Program name used in error messages */
static char *Prog;
struct passwd pwent;

/*
 * External identifiers
 */

extern char **newenvp;
extern char **environ;
extern size_t newenvc;

/* local function prototypes */
static char *basename(char *str)
{
	char *cp = strrchr(str, '/');
	return cp ? cp + 1 : str;
}

/*
 * setup_uid_gid() split in two functions for PAM support -
 * pam_setcred() needs to be called after initgroups(), but
 * before setuid().
 */
static int setup_groups(const struct passwd *info)
{
	/*
	 * Set the real group ID to the primary group ID in the password
	 * file.
	 */
	if (setgid(info->pw_gid) == -1) {
		perror("setgid");
		syslog(LOG_ERR, "bad group ID `%d' for user `%s': %m\n",
		        info->pw_gid, info->pw_name);
		closelog();
		return -1;
	}

	/*
	 * For systems which support multiple concurrent groups, go get
	 * the group set from the /etc/group file.
	 */
	if (initgroups(info->pw_name, info->pw_gid) == -1) {
		perror("initgroups");
		syslog(LOG_ERR, "initgroups failed for user `%s': %m\n",
		        info->pw_name);
		closelog();
		return -1;
	}
	return 0;
}

int change_uid(const struct passwd *info)
{
	/*
	 * Set the real UID to the UID value in the password file.
	 */
	if (setuid (info->pw_uid)) {
		perror("setuid");
		syslog(LOG_ERR, "bad user ID `%d' for user `%s': %m\n",
		        (int) info->pw_uid, info->pw_name);
		closelog();
		return -1;
	}
	return 0;
}

/*
 * Cleanup ~/.xsession-errors.
 *
 * The function looks for .xsession-errors in the current directory, so when it
 * is called the current directory must be the user's homedir.
 *
 * The function also assumes that we are running as the user. As a consequence
 * it does not worry about symlink attacks, because they would only be possible
 * if the user's home directory is group or world writable.
 *
 * curdirname is the name of the current directory, and it is only used when
 * logging error messages.
 *
 * The function returns true on success, false on failure.
 */
int cleanup_xse(off_t maxsize, const char* curdirname)
{
	int ret = 0;
	int xse_fd = -1;
	struct stat xse_st;

	xse_fd = open(".xsession-errors", O_WRONLY | O_CREAT, 0600);
	if (xse_fd < 0) {
		perror("open ~/.xsession-errors");
		syslog(LOG_ERR, "cannot open `%s/%s': %m\n", curdirname, ".xsession-errors");
		goto cleanup;
	}
	if (fstat(xse_fd, &xse_st) < 0) {
		perror("stat ~/.xsession-errors");
		syslog(LOG_ERR, "cannot stat `%s/%s': %m\n", curdirname, ".xsession-errors");
		goto cleanup;
	}
	if (xse_st.st_size > maxsize) {
		if (ftruncate(xse_fd, 0) < 0) {
			perror("truncating ~/.xsession-errors");
			syslog(LOG_ERR, "cannot truncate `%s/%s': %m\n", curdirname, ".xsession-errors");
			goto cleanup;
		}
	}

	/* If we made it so far, we succeeded */
	ret = 1;

cleanup:
	if (xse_fd >= 0) {
		close(xse_fd);
	}
	return ret;
}

static int try_vtstate(const char* dev, struct vt_stat* vtstat)
{
	int res = 0;
	int fd = open(dev, O_WRONLY | O_NOCTTY, 0);
	if (fd < 0) {
		goto cleanup;
	}
	if (ioctl(fd, VT_GETSTATE, vtstat) < 0) {
		goto cleanup;
	}
	res = 1;

cleanup:
	if (fd >= 0) {
		close(fd);
	}
	return res;
}

static int get_vtstate(struct vt_stat* vtstat)
{
	if (try_vtstate("/dev/tty", vtstat)) {
		return 1;
	}
	if (try_vtstate("/dev/tty0", vtstat)) {
		return 1;
	}
	if (try_vtstate("/dev/console", vtstat)) {
		return 1;
	}
	return 0;
}

/*
 * Allocate a new vt, open it and return the file descriptor and the vt number.
 *
 * Searching the vt starts at the initial value of vtnum.
 */
int open_vt(int *vtnum)
{
	char vtname[15];
	int res = -1;
	struct vt_stat vtstat;
	unsigned short vtmask;

	if (!get_vtstate(&vtstat)) {
		fprintf(stderr, _("%s: cannot find or open the console\n"), Prog);
		goto cleanup;
	}

	for (vtmask = 1 << *vtnum; vtstat.v_state & vtmask; ++*vtnum, vtmask <<= 1)
		;
	if (!vtmask) {
		fprintf(stderr, _("%s: all VTs seem to be busy\n"), Prog);
		goto cleanup;
	}

	snprintf(vtname, 15, "/dev/tty%d", *vtnum);

	res = open(vtname, O_RDWR | O_NOCTTY, 0);
	if (res < 0) {
		fprintf(stderr, _("%s: cannot open %s: %m\n"), Prog, vtname);
		goto cleanup;
	}

cleanup:
	return res;
}

/* Signal handler for parent process later */
static void catch_signals(int sig)
{
	++caught;
}

/* This I ripped out of su.c from sh-utils after the Mandrake pam patch
 * have been applied.  Some work was needed to get it integrated into
 * su.c from shadow.
 */
static int run_shell(const char** args, int* status)
{
	int child;
	sigset_t ourset;
	struct sigaction action;

	child = fork();
	if (child == 0) {	/* child shell */
		/*
		 * This is a workaround for Linux libc bug/feature (?) - the
		 * /dev/log file descriptor is open without the close-on-exec flag
		 * and used to be passed to the new shell. There is "fcntl(LogFile,
		 * F_SETFD, 1)" in libc/misc/syslog.c, but it is commented out (at
		 * least in 5.4.33). Why?  --marekm
		 */
		closelog();

		/*
		 * PAM_DATA_SILENT is not supported by some modules, and
		 * there is no strong need to clean up the process space's
		 * memory since we will either call exec or exit.
		pam_end(pamh, PAM_SUCCESS | PAM_DATA_SILENT);
		 */
		//syslog(LOG_WARNING, "Execute %s %s %s %s", args[0], args[1], args[2], args[3]);
		(void) execv(args[0], (char **) args);
		exit(errno == ENOENT ? E_CMD_NOTFOUND : E_CMD_NOEXEC);
	} else if (child == -1) {
		(void) fprintf(stderr, "%s: Cannot fork user shell\n", Prog);
		syslog(LOG_WARNING, "Cannot execute %s", args[0]);
		closelog();
		return 1;
	}

	/* parent only */

	/* Reset caught signal flag */
	caught = 0;

	/* Block all signals */
	sigfillset(&ourset);
	if (sigprocmask(SIG_BLOCK, &ourset, NULL)) {
		(void) fprintf(stderr, "%s: signal malfunction\n", Prog);
		goto killed;
	}

	/* Catch SIGTERM and SIGALRM using 'catch_signals' */
	action.sa_handler = catch_signals;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigemptyset(&ourset);

	if (sigaddset(&ourset, SIGTERM)
#ifdef DEBUG_DM
	                || sigaddset (&ourset, SIGINT)
	                || sigaddset (&ourset, SIGQUIT)
#endif
	                || sigaddset (&ourset, SIGALRM)
	                || sigaction (SIGTERM, &action, NULL)
	                || sigprocmask (SIG_UNBLOCK, &ourset, NULL)
	   ) {
		fprintf(stderr, "%s: signal masking malfunction\n", Prog);
		goto killed;
	}

	do {
		int pid;

		pid = waitpid(-1, status, WUNTRACED);

		if (WIFSTOPPED(*status)) {
			kill(getpid(), SIGSTOP);
			/* once we get here, we must have resumed */
			kill(pid, SIGCONT);
		}
	} while (WIFSTOPPED(*status));

	/* Unblock signals */
	sigfillset(&ourset);
	if (sigprocmask(SIG_UNBLOCK, &ourset, NULL)) {
		(void) fprintf(stderr, "%s: signal malfunction\n", Prog);
		goto killed;
	}

	if (caught) {
		goto killed;
	}

	return 0;

killed:
	fprintf(stderr, "\nSession terminated, killing shell...");
	kill(child, SIGTERM);
	sleep(2);
	kill(child, SIGKILL);
	fprintf(stderr, " ...killed.\n");
	return -1;
}

/*
 * Run the X session
 *
 * @param xinit
 *   The path to xinit
 * @param xsession
 *   The path to the X session
 * @param xoptions
 *   The path to the X options (can be NULL if no options are to be passed)
 * @param mst
 *   The minimum time (in seconds) that a session should last to be considered
 *   successful
 */
void run_and_restart(const char* xinit, const char* xsession, const char* xoptions, int mst)
{
	static int retry_times[] = { 0, 0, 30, 30, 60, 60, -1 };
	int restart_count = 0;
	char command[BUFSIZ];
	const char* args[4];

	if (xoptions != NULL) {
		snprintf(command, BUFSIZ, "exec %s %s -- %s", xinit, xsession, xoptions);
	} else {
		snprintf(command, BUFSIZ, "exec %s %s", xinit, xsession);
	}
	command[BUFSIZ-1] = 0;

	args[0] = "/bin/sh";
	args[1] = "-c";
	args[2] = command;
	args[3] = 0;

	while (1) {
		/* Run the X server */
		time_t begin = time(NULL);
		time_t end;
		int status;
		syslog(LOG_INFO, "Running %s %s '%s'", args[0], args[1], args[2]);
		if (run_shell(args, &status)) {
			return;
		}
		end = time(NULL);

		/* Check if the session was too short */
		if (end - begin < mst) {
			syslog(LOG_WARNING, "Session for %s was shorter than %d seconds: possible problems", name, mst);
			if (retry_times[restart_count+1] != -1) {
				++restart_count;
			}
		} else {
			restart_count = 0;
		}

		/* Sleep a bit if the session was too short */
		sleep(retry_times[restart_count]);
		syslog(LOG_INFO, "Restarting session for %s", name);
		return;	// add!!
	}
}

/*
 * Copy from the environment the value of $name into dest.
 *
 * If $name is not in the environment, use def.
 *
 * @param destination buffer, should be at least BUFSIZ long
 * @param name name of the environment variable to look up
 * @param def default value to use if $name is not found
 */
static char *string_from_env(char* dest, const char* name, const char* def)
{
	char* cp = getenv(name);
	if (cp != NULL) {
		strncpy(dest, cp, BUFSIZ-1);
	} else {
		strncpy(dest, def, BUFSIZ-1);
	}
	dest[BUFSIZ-1] = 0;
	return cp;
}

static void monitor_cmdline_help(int argc, char** argv, FILE* out)
{
	fprintf(out, "Usage: %s [options]\n\n", argv[0]);
	fprintf(out, "Options:\n");
	fprintf(out, " --help         print this help message\n");
	fprintf(out, " --version      print %s's version number\n", NAME);
	fprintf(out, " --session=cmd  run cmd instead of %s\n", DM_SESSION);
	fprintf(out, "                (use for testing)\n");
}

/*
 * Start the monitor, that will continue to rerun xinit with appropriate delays
 */
static int dm_monitor(int argc, char **argv)
{
	static int opt_help = 0;
	static int opt_version = 0;
	static struct option options[] = {
		/* These options set a flag. */
		{"help",    no_argument,       &opt_help, 1},
		{"version", no_argument,       &opt_version, 1},
		{"session", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};
	const char* opt_session = DM_SESSION;
	char xinit[BUFSIZ];
	char xoptions[BUFSIZ];
	char xoptions1[BUFSIZ];
	char* cp;
	int mst;
	int vt_fd = -1;
	int vt_num;

	/* Parse command line options */
	while (1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "s:", options, &option_index);
		if (c == -1) {
			break;
		}
		switch (c) {
		case 0:
			break;
		case 's':
			opt_session = optarg;
			break;
		default:
			fprintf(stderr, "Invalid command line option\n");
			monitor_cmdline_help(argc, argv, stderr);
			return 1;
		}
	}
	if (opt_help) {
		monitor_cmdline_help(argc, argv, stdout);
		return 0;
	}
	if (opt_version) {
		printf("%s version %s\n", NAME, VERSION);
		return 0;
	}

	/* We only run if we are root */
	if (getuid() != 0) {
		fprintf(stderr, _("%s: can only be run by root\n"), Prog);
		return E_NOPERM;
	}

	syslog(LOG_INFO, "Starting dm monitor");

	{
		char startvt[BUFSIZ];
		string_from_env(startvt, "DM_FIRST_VT", "7");
		vt_num = strtoul(startvt, NULL, 10);
	}
	if ((vt_fd = open_vt(&vt_num)) == -1) {
		fprintf(stderr, _("%s: cannot allocate a virtual terminal\n"), Prog);
		return 1;
	}

	/* Read the configuration from the environment */
	cp = getenv("DM_MIN_SESSION_TIME");
	mst = cp ? atoi(cp) : 60;

	setenv("DM_RUN_SESSION", "1", 1);
	if (getenv("DM_XSESSION")) {
		// /bin/sh -c 'exec xinit berry-dm -- vt7'
		string_from_env(xinit, "DM_XINIT", "/usr/bin/xinit");
		string_from_env(xoptions, "DM_X_OPTIONS", "");

		if (xoptions[0] == 0) {
			snprintf(xoptions1, BUFSIZ, "vt%d", vt_num);
		} else {
			snprintf(xoptions1, BUFSIZ, "vt%d %s", vt_num, xoptions);
		}

		run_and_restart(xinit, opt_session, xoptions1, mst);
	} else {
		//string_from_env(xinit, "DM_INIT", "/usr/bin/weston");
		string_from_env(xinit, "DM_XINIT", "/usr/bin/weston");
		run_and_restart(xinit, "", 0, mst);
	}

	close(vt_fd);

	return 0;
}

/*
 * Start the session, with proper autologin and pam handling
 */
static int dm_session(int argc, char **argv)
{
	int ret;
	char *cp;
	char **envcp;
	const char *tty = 0;	/* Name of tty SU is run from        */
	struct passwd *pw = 0;
	int status;
	char xsession[BUFSIZ];
	const char* args[5];

	/* We only run if we are root */
	if (getuid() != 0) {
		fprintf(stderr, _("%s: can only be run by root\n"), Prog);
		return E_NOPERM;
	}

	string_from_env(xsession, "DM_XSESSION", "/etc/X11/Xsession");

	/*
	 * Get the tty name. Entries will be logged indicating that the user
	 * tried to change to the named new user from the current terminal.
	 */
	if (isatty(0) && (cp = ttyname(0))) {
		if (strncmp(cp, "/dev/", 5) == 0) {
			tty = cp + 5;
		} else {
			tty = cp;
		}
	} else {
		tty = "???";
	}

	/* Get the user we should run the session for */
	if (getenv("DM_USER") == NULL) {
		strcpy(name, "root");
	} else {
		STRFCPY(name, getenv("DM_USER"));
	}

	ret = pam_start(NAME, name, &conv, &pamh);
	if (ret != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_start: error %d", ret);
		fprintf(stderr, _("%s: pam_start: error %d\n"),
		         Prog, ret);
		exit(1);
	}

	ret = pam_set_item(pamh, PAM_TTY, (const void *) tty);
	if (ret == PAM_SUCCESS) {
		ret = pam_set_item(pamh, PAM_RUSER, (const void *) "root");
	}
	if (ret != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_set_item: %s",
		        pam_strerror(pamh, ret));
		fprintf(stderr, _("%s: %s\n"), Prog, pam_strerror(pamh, ret));
		pam_end(pamh, ret);
		exit(1);
	}

	/*
	 * Validate the user using the normal system user database
	 */
	if (!(pw = getpwnam(name))) {
		(void) fprintf(stderr, _("Unknown id: %s\n"), name);
		closelog();
		exit(1);
	}
	pwent = *pw;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	/* FIXME: should we ignore this, or honour it?
	 * this can fail if the current user's account is invalid. "This
	 * includes checking for password and account expiration, as well as
	 * verifying access hour restrictions."
	 */
	ret = pam_acct_mgmt(pamh, 0);
	if (ret != PAM_SUCCESS) {
		fprintf(stderr, _("%s: %s\n(Ignored)\n"), Prog,
		         pam_strerror(pamh, ret));
	}

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);

	/* save SU information */
	syslog(LOG_INFO, "Successful su for %s by %s", name, "root");

#ifdef USE_SYSLOG
	syslog(LOG_INFO, "+ %s %s:%s", tty,
	        "root", name[0] ? name : "???");
#endif

	/* set primary group id and supplementary groups */
	if (setup_groups(&pwent)) {
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}

	/*
	 * pam_setcred() may do things like resource limits, console groups,
	 * and much more, depending on the configured modules
	 */
	ret = pam_setcred(pamh, PAM_ESTABLISH_CRED);
	if (ret != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_setcred: %s", pam_strerror(pamh, ret));
		fprintf(stderr, _("%s: %s\n"), Prog, pam_strerror(pamh, ret));
		pam_end(pamh, ret);
		exit(1);
	}

	ret = pam_open_session(pamh, 0);
	if (ret != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_open_session: %s",
		        pam_strerror(pamh, ret));
		fprintf(stderr, _("%s: %s\n"), Prog, pam_strerror(pamh, ret));
		pam_setcred(pamh, PAM_DELETE_CRED);
		pam_end(pamh, ret);
		exit(1);
	}

	/* update environment with all pam set variables */
	envcp = pam_getenvlist(pamh);
	if (envcp) {
		while (*envcp) {
			putenv(*envcp);
			envcp++;
		}
	}

	/* become the new user */
	if (change_uid(&pwent)) {
		pam_close_session(pamh, 0);
		pam_setcred(pamh, PAM_DELETE_CRED);
		pam_end(pamh, PAM_ABORT);
		exit(1);
	}

	setenv("HOME", pwent.pw_dir, 1);
	setenv("USER", pwent.pw_name, 1);
	setenv("USERNAME", pwent.pw_name, 1);
	setenv("LOGNAME", pwent.pw_name, 1);
	setenv("PWD", pwent.pw_dir, 1);
	setenv("SHELL", pwent.pw_shell, 1);

	// Variables that gdm sets but we do not:
	//
	// This is something that we should see how to handle.
	// What I know so far is:
	//  - It should point to ~/.Xauthority, which should exist.
	//  - 'xauth generate' should be able to create it if missing, but I
	//    have not tested it yet.
	// g_setenv("XAUTHORITY", d->userauth, TRUE);
	//
	// This is 'gnome', 'kde' and so on, and should probably be set by the
	// X session script:
	// g_setenv("DESKTOP_SESSION", session, TRUE);
	//
	// This looks gdm specific:
	// g_setenv("GDMSESSION", session, TRUE);

	// Variables that gdm sets but we delegate other tools to set:
	//
	// These are set by xinit
	// g_setenv("DISPLAY", d->name, TRUE);
	// g_setenv("WINDOWPATH", d->windowpath, TRUE);
	//
	// This is set by the pam_getenvlist loop above
	// g_setenv("XDG_SESSION_COOKIE", ck_session_cookie, TRUE);
	//
	// This is set by "sh -l" from /etc/profile
	// if (pwent->pw_uid == 0)
	//   g_setenv("PATH", gdm_daemon_config_get_value_string (GDM_KEY_ROOT_PATH), TRUE);
	// else
	//   g_setenv("PATH", gdm_daemon_config_get_value_string (GDM_KEY_PATH), TRUE);
	//

	/* Clear the DM_* environment variables */
	unsetenv("DM_USER");
	unsetenv("DM_XINIT");
	unsetenv("DM_XSESSION");
	unsetenv("DM_X_OPTIONS");
	unsetenv("DM_MIN_SESSION_TIME");
	unsetenv("DM_RUN_SESSION");

	if (chdir(pwent.pw_dir) == 0)
		/* Truncate ~/.xsession-errors */
	{
		cleanup_xse(0, pwent.pw_dir);
	}

	args[0] = "/bin/sh";
	args[1] = "-l";
	args[2] = "-c";
	args[3] = xsession;
	args[4] = NULL;

	run_shell(args, &status);

	ret = pam_close_session(pamh, 0);
	if (ret != PAM_SUCCESS) {
		syslog(LOG_ERR, "pam_close_session: %s",
		        pam_strerror(pamh, ret));
		fprintf(stderr, _("%s: %s\n"), Prog, pam_strerror(pamh, ret));
		pam_end(pamh, ret);
		return 1;
	}

	ret = pam_end(pamh, PAM_SUCCESS);

	return status;
}

/* Return true if string ends with substring */
static int ends_with(const char* string, const char* substring)
{
	int len_string = strlen(string);
	int len_substring = strlen(substring);
	if (len_string < len_substring) {
		return 0;
	}
	return strcmp(string + (len_string - len_substring), substring) == 0;
}

/*
 * dm - start X with autologin to a given user
 *
 * First, X is started as root, and dm itself is used as the session.
 *
 * When run as the session, dm performs a proper login to a given user and
 * starts the X session.
 */
//int main(int argc, char **argv)
int dm_main(int argc, char **argv)
{
	int ret;

	/*
	 * Get the program name. The program name is used as a prefix to
	 * most error messages.
	 */
	Prog = basename(argv[0]);

	openlog(NAME, LOG_PID, LOG_AUTHPRIV);

	/*
	 * Process the command line arguments.
	 */

	// TODO command line processing

	if (getenv("DM_RUN_SESSION") != NULL) {
		syslog(LOG_INFO, "Starting dm X session");
		ret = dm_session(argc, argv);
	} else {
		ret = dm_monitor(argc, argv);
	}

	closelog();
	return ret;
}


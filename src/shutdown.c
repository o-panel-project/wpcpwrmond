/*
 * $Id: shutdown.c 66 2011-10-26 08:07:06Z r-hara $
 * Based on sysvinit-2.86 shutdown.c
 */
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> 
#include <signal.h>
#include <fcntl.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>

extern void _tctl_SetBackLight(int);
extern int _tctl_GetBackLight();
extern void _tctl_poweroff();

/* char *Version = "@(#) shutdown 2.86-1 31-Jul-2004 miquels@cistron.nl"; */

#define MESSAGELEN	256

char message[MESSAGELEN];	/* Warning message	*/
char newstate[64];	/* What are we gonna do		*/

char *clean_env[] = {
	"HOME=/",
	"PATH=/bin:/usr/bin:/sbin:/usr/sbin",
	"TERM=dumb",
	NULL,
};

extern char **environ;

/*
 *	Tell everyone the system is going down in 'mins' minutes.
 */
void warn()
{
	char buf[MESSAGELEN + sizeof(newstate)];
	int len;

	buf[0] = 0;
	strncat(buf, message, sizeof(buf) - 1);
	len = strlen(buf);

	snprintf(buf + len, sizeof(buf) - len,
			"\rThe system is going down %s NOW!\r\n",
			newstate);
	fprintf(stderr, buf);
}

/*
 *	Spawn an external program.
 */
int spawn(int noerr, char *prog, ...)
{
	va_list	ap;
	pid_t	pid, rc;
	int	i;
	char	*argv[8];

	i = 0;
	while ((pid = fork()) < 0 && i < 10) {
		perror("fork");
		sleep(5);
		i++;
	}

	if (pid < 0) return -1;

	if (pid > 0) {
		while((rc = wait(&i)) != pid)
			if (rc < 0 && errno == ECHILD)
				break;
		return (rc == pid) ? WEXITSTATUS(i) : -1;
	}

	if (noerr) fclose(stderr);

	argv[0] = prog;
	va_start(ap, prog);
	for (i = 1; i < 7 && (argv[i] = va_arg(ap, char *)) != NULL; i++)
		;
	argv[i] = NULL;
	va_end(ap);

	chdir("/");
	environ = clean_env;

	execvp(argv[0], argv);
	perror(argv[0]);
	exit(1);

	/*NOTREACHED*/
	return 0;
}

/*
 *	Kill all processes, call /etc/init.d/halt (if present)
 */
void fastdown()
{
	int i;

	/* First close all files. */
	for(i = 0; i < 3; i++)
		if (!isatty(i)) {
			close(i);
			open("/dev/null", O_RDWR);
		}
	for(i = 3; i < 20; i++) close(i);
	close(255);

	/* First idle init. */
	kill(1, SIGTSTP);

	i = _tctl_GetBackLight();
	_tctl_SetBackLight(0);	/* backlight off */
	system("echo 0 > /sys/devices/platform/omapdss/overlay2/global_alpha");
	system("echo 0 > /sys/devices/platform/omapdss/manager0/alpha_blending_enabled");
	{
	const char *ttyname = "/dev/tty1";
	const char *message = "Power down ...";
	char cmd[256];
	struct winsize arg;
	int x=0, y=0;
	int fd = open(ttyname, O_RDWR);
	if (fd >= 0) {
		if (!ioctl(fd, TIOCGWINSZ, &arg)) {
			x = arg.ws_col;
			y = arg.ws_row;
		}
		close(fd);
	}
	sprintf(cmd, "tput clear > %s", ttyname);
	system(cmd);
	sprintf(cmd, "tput cup %d %d > %s", y/2, (x-strlen(message))/2, ttyname);
	system(cmd);
	sprintf(cmd, "echo \'%s\' > %s", message, ttyname);
	system(cmd);
	}
	_tctl_SetBackLight(i);	/* backlight on */

	/* Kill all processes. */
	fprintf(stderr, "shutdown: sending all processes the TERM signal...\r\n");
	kill(-1, SIGTERM);
	sleep(3);
	fprintf(stderr, "shutdown: sending all processes the KILL signal.\r\n");
	(void) kill(-1, SIGKILL);

	/* script failed or not present: do it ourself. */
	sleep(1); /* Give init the chance to collect zombies. */

	sync();
	fprintf(stderr, "shutdown: turning off swap\r\n");
	spawn(0, "swapoff", "-a", NULL);
	fprintf(stderr, "shutdown: unmounting user file systems\r\n");
	spawn(0, "umount", "/config", NULL);
	spawn(0, "umount", "/opt/extmem", NULL);
	spawn(0, "umount", "/opt", NULL);	/* menu */
	spawn(0, "umount", "/mnt1", NULL);	/* selfcheck */
	fprintf(stderr, "shutdown: remount ro root file system\r\n");
	spawn(0, "mount", "-o", "remount,ro", "/", NULL);

	/* We're done, reboot now. */
	fprintf(stderr, "Please stand by while power down the system.\r\n");
	fflush(stderr);
	sleep(1);
	_tctl_poweroff();
	exit(0);
}

/*
 *	Go to runlevel 6.
 */
void my_shutdown(char *halttype)
{
	/* Warn for the last time */
	warn(0);

	openlog("tctlreboot", LOG_PID, LOG_USER);
	syslog(LOG_NOTICE, "shutting down for system reboot");
	closelog();

	/* See if we have to do it ourself. */
	fastdown();

	/* Oops - failed. */
	fprintf(stderr, "\rtctlreboot: cannot execute.\r\n");
	exit(1);
}

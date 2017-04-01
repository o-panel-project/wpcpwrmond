/*
 * $Id: shutdown.c 66 2011-10-26 08:07:06Z r-hara $
 * Based on sysvinit-2.86 shutdown.c
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h> 
#include <signal.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>

extern void _tctl_pwrmonitor_ready(int);
extern void my_shutdown(char *);
extern char newstate[];

static int do_terminate = 0;

#define SYSCALL(call) while (((call) == -1) && (errno == EINTR))

/*
 *  Break off an already running shutdown.
 */
void stopit(int sig)
{
	printf("\r\nShutdown cancelled.\r\n");
	exit(0);
}

/*
 *	Main program.
 *	Process the options and do the final countdown.
 */
static int shutdown_main(int argc, char **argv)
{
	struct sigaction	sa;
	char	*halttype = NULL;

	/* Tell users what we're gonna do. */
	strcpy(newstate, "for tctlreboot");

	/*
	 *	Catch some common signals.
	 */
	signal(SIGQUIT, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP,  SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = stopit;
	sigaction(SIGINT, &sa, NULL);

	/* Shutdown NOW */
	my_shutdown(halttype);

	_tctl_pwrmonitor_ready(0);
	exit(EXIT_FAILURE); /* Never happens */
}

static void on_terminate(int signal)
{
	fprintf(stderr, "wpcpwrmond caught signal %d, terminate.\n", signal);
	do_terminate = 1;
}

static void set_signal_handler()
{
	sigset_t mask;
	sigemptyset(&mask);
	signal(SIGTERM, on_terminate);
	signal(SIGINT, on_terminate);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

int main(int argc, char *argv[])
{
	uid_t	realuid;
	char *opt_event_file = strdup("/dev/input/wpc_pwrbutton0");
	int opt, fd, nread;
	struct stat fs;
	struct input_event ev;

	/* We can be installed setuid root (executable for a special group) */
	realuid = getuid();
	setuid(geteuid());

	if (getuid() != 0) {
  		fprintf(stderr, "shutdown: you must be root to do that!\n");
  		exit(1);
	}

	/* parse option */
	while ((opt = getopt(argc, argv, "i:")) != -1) {
		switch (opt) {
		case 'i':
			/* alternate key event */
			free(opt_event_file);
			opt_event_file = strdup(optarg);
			break;
		default:
			fprintf(stderr, "Usage: %s [-i device]\n", argv[0]);
			free(opt_event_file);
			return -1;
		}
	}

	set_signal_handler();

	/* Go to the root directory */
	chdir("/");

	fd = open(opt_event_file, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "error: could not open device\n");
		free(opt_event_file);
		return -1;
	}
	if (fstat(fd, &fs)) {
		fprintf(stderr, "error: could not stat the device\n");
		goto exit_lbl;
	}
	if (fs.st_rdev && ioctl(fd, EVIOCGRAB, 1)) {
		fprintf(stderr, "error: could not grab the device\n");
		goto exit_lbl;
	}

	_tctl_pwrmonitor_ready(1);

	while (!do_terminate) {
		struct pollfd fds = {fd, POLLIN, 0};
		nread = poll(&fds, 1, 5000);
		if (nread <= 0)
			continue;
		nread = read(fd, &ev, sizeof(ev));
		if (nread == -1 && errno == EINTR) {
			fprintf(stderr, "error: read interrupt try again.\n");
			continue;
		}
		if (nread != sizeof(ev))
			continue;
		if (ev.type == EV_KEY && ev.code == KEY_POWER && ev.value == 1)
			shutdown_main(argc, argv);
		//	printf("\r\nEntering shutdown process.\r\n");
	}

	if (fs.st_rdev)
		ioctl(fd, EVIOCGRAB, 0);

exit_lbl:
	close(fd);
	free(opt_event_file);
	_tctl_pwrmonitor_ready(0);

	return 0;
}

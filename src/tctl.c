/*
 * Backlight control
 * $Id: tctl.c 10 2011-09-20 08:57:25Z r-hara $
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include "tctlwpcio.h"

#define SYSFS_BACKLIGHT_IF_DIR	\
	"/sys/class/backlight/backlight.2"
#define SYSFS_BACKLIGHT_IF_DIR_J4	\
	"/sys/class/backlight/backlight.4"
#define	SYSFS_BACKLIGHT_BRIGHT	SYSFS_BACKLIGHT_IF_DIR"/brightness"
#define	SYSFS_BACKLIGHT_BRIGHT_J4	SYSFS_BACKLIGHT_IF_DIR_J4"/brightness"
#define SYSFS_MONITOR_READY "/sys/class/wpc_pwrbutton/monitor_ready"

static inline int sysfs_read(const char *f)
{
	FILE *fp;
	int val;

	if ((fp = fopen(f, "r")) == NULL) {
		perror("fopen");
		return -1;
	}

	if (fscanf(fp, "%d\n", &val) == EOF) {
		perror("fscanf");
		val = -1;
	}

	fclose(fp);
	return val;
}

static inline int sysfs_write(const char *f, char *d, size_t s)
{
	int fd, written;

	if ((fd = open(f, O_WRONLY)) < 0) {
		perror("open");
		return -1;
	}

	written = write(fd, d, s);
	if (s != written) {
		perror("write");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static void BackLight_brightness_set(int val)
{
	char buf[128];
	sprintf(buf, "%d\n", val);
	if (sysfs_write(SYSFS_BACKLIGHT_BRIGHT, (char*)buf, strlen(buf)) < 0)
		sysfs_write(SYSFS_BACKLIGHT_BRIGHT_J4, (char*)buf, strlen(buf));
}

static int BackLight_brightness_get()
{
	int val;
	if ((val = sysfs_read(SYSFS_BACKLIGHT_BRIGHT)) < 0)
		return sysfs_read(SYSFS_BACKLIGHT_BRIGHT_J4);
	return val;
}

static void Monitor_ready_set(int val)
{
	char buf[128];
	sprintf(buf, "%d\n", val);
	sysfs_write(SYSFS_MONITOR_READY, (char*)buf, strlen(buf));
}

/*
 * API
 */

/*
 * Set brightness
 */
void _tctl_SetBackLight(int val)
{
	BackLight_brightness_set(val);
	return;
}

/*
 * Get brightness
 */
int _tctl_GetBackLight()
{
	int val;
	val = BackLight_brightness_get();
	if (val < 0) {
		BackLight_brightness_set(50);
		return 50;
	}
	return val;
}

/*
 * Terminal power control
 */
void _tctl_poweroff()
{
    int fd;

    if ((fd = open(TCTL_WPCIO_DEVICE, O_RDWR)) < 0) {
        return;
    }
    if (ioctl(fd, WPC_SET_GPIO_OUTPUT_HIGH, 116) < 0) {
        close(fd);
        return;
    }
    /* not reached */
    close(fd);
}

void _tctl_pwrmonitor_ready(int val)
{
	Monitor_ready_set(val);
	return;
}


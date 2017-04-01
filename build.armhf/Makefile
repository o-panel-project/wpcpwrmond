CROSS_COMPILE = arm-linux-gnueabihf-

ifdef CROSS_COMPILE
  CC = $(CROSS_COMPILE)gcc
  SYSROOT=/opt/arm-dev/armhf
  PKGCONFIG=../scripts/pkg-config.sh
  CFLAGS += -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mhard-float
  CFLAGS += -mfloat-abi=hard -ftree-vectorize -mvectorize-with-neon-quad
  CFLAGS += -mthumb-interwork
  STRIP = ${CROSS_COMPILE}strip
  STRIP_OPT = --remove-section=.comment --remove-section=.note
else
  CC = gcc
  SYSROOT=
  PKGCONFIG=/usr/bin/pkg-config
  STRIP = strip
  STRIP_OPT =
endif

PROGRAM	= wpcpwrmond
SRCS	= main.c shutdown.c tctl.c version.c
OBJS	= ${SRCS:%.c=%.o}

VPATH = ../src

CFLAGS += -Wall -O2
CFLAGS += -I../include --sysroot=$(SYSROOT)

LDFLAGS += --sysroot=$(SYSROOT) -static

all: depend.inc $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	$(STRIP) $(STRIP_OPT) $@

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	-rm -rf $(OBJS) $(PROGRAM) depend.inc

depend.inc: $(SRCS)
	$(CC) -MM $(CFLAGS) $^ > depend.inc

-include depend.inc

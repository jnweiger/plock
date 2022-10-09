# The clock does 15 tickes, speed of the clock:
# -DSEC_PER_TICK=60		15 Minutes
# -DSEC_PER_TICK=120		30 Minutes
# -DSEC_PER_TICK=180		45 Minutes
SPEED=60

CC = gcc
CFLAGS = -O6 -Wall -pipe -I/usr/include/X11 -DSEC_PER_TICK=$(SPEED)
X11LIB = -L/usr/lib/X11 -lX11 -lcrypt
LDFLAGS = #-static

OBJS = plock.o anim.o image.o XLoadRaster.o explode.o snd.o

plock: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(X11LIB)

plock.o: plock.h config.h GNUmakefile
anim.o: plock.h
image.o: plock.h blit.c
XLoadRaster.o: ras.h
explode.o: plock.h
snd.o: plock.h
plock.h:ras.h

clean:
	rm -f $(OBJS) plock core

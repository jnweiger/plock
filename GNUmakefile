CC = gcc
CFLAGS = -O -Wall -pipe -I/local/X11/include #-DSEC_PER_TICK=120
X11LIB = -L/local/X11R5/lib -lX11 
LDFLAGS = -Bstatic

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

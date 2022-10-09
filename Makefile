CC = cc
#CFLAGS = -O -pipe -I/local/X11/include
X11LIB = -L/local/X11/lib -lX11 #-L. -lnmalloc

CFLAGS = -O -I/local/X11/include #-DSEC_PER_TICK=120 -DDEBUG
#X11LIB = -lX11 #-L. -lnmalloc
LDFLAGS = #-Bstatic
OBJS = plock.o anim.o image.o XLoadRaster.o explode.o snd.o #localtime.o

plock: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(X11LIB) 

plock.o: plock.h config.h
anim.o: plock.h
image.o: plock.h blit.c
XLoadRaster.o: ras.h
explode.o: plock.h
snd.o: plock.h
plock.h:ras.h

clean:
	rm -f $(OBJS) plock core

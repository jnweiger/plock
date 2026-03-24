#
# Debian Requires:
#   	apt install libasound2-dev
#
# Suse Requires:
#	zypper install alsa-devel
#
all: anim_ras

CC = gcc
CFLAGS = -g -O0 -Wno-old-style-definition -std=gnu89 -Wno-implicit-function-declaration

%.o: %.c
	$(CC) $(CFLAGS) -c $<

sound:
	cc    -DSTANDALONE=1  play_alsa.c   -o play_alsa -l asound
	./play_alsa < ../rasterfiles/sound/new_level.snd
	./play_alsa < ../rasterfiles/sound/open_the_box.snd
	./play_alsa < ../rasterfiles/sound/lets_go.snd
	./play_alsa < ../rasterfiles/sound/oh_no.snd
	./play_alsa < ../rasterfiles/sound/explode.snd


loadras: ../XLoadRaster.o loadras.c
	cc -g -O0 -Wno-old-style-definition -DSTANDALONE=1 loadras.c ../XLoadRaster.o -o loadras -lX11
	./loadras ../rasterfiles/catras/clock.ras &
	./loadras ../rasterfiles/catras/bombers.ras


anim_ras: ../image.o ../XLoadRaster.o anim_ras.o loadras.o
	cc -g -O0 -Wno-old-style-definition anim_ras.o loadras.o ../image.o ../XLoadRaster.o -o anim_ras -lX11


l1_list: l1_list.c
	cc     l1_list.c   -o l1_list -lasound
l2_open: l2_open.c
	cc     l2_open.c   -o l2_open -lasound
l3_play: l3_play.c
	cc     l3_play.c   -o l3_play -lasound
l4_rec: l4_rec.c
	cc     l4_rec.c   -o l4_rec -lasound

clean:
	rm -f l1_list l2_open l3_play l4_rec
	rm -f loadras play_alsa anim_ras
	rm -rf *.o ../*.o

all: loadras

sound:
	cc     play_plock.c   -o play_plock -l asound
	./play_plock < ../rasterfiles/sound/new_level.snd
	./play_plock < ../rasterfiles/sound/open_the_box.snd
	./play_plock < ../rasterfiles/sound/lets_go.snd
	./play_plock < ../rasterfiles/sound/oh_no.snd
	./play_plock < ../rasterfiles/sound/explode.snd

loadras: ../XLoadRaster.o loadras.c
	cc     loadras.c  ../XLoadRaster.o -o loadras -lX11

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
	rm -f loadras play_plock

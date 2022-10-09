#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include "ras.h"

#ifdef __GNUC__
# define __P(s) s
# ifdef sun
#  include "sun_stdlib.h"
# endif
#else
# define __P(s) ()
#endif

#ifdef linux
#define syslog(a,b)
#define MAXPID 32767
#endif /* linux */

#include <sys/param.h>
#define Free(a) {if ((a) == 0) abort(); else free((void *)(a)); (a)=0;}

#ifdef DEBUG
# define debug(a) printf(a)
# define debug1(a, b) printf(a, b)
# define debug2(a, b, c) printf(a, b, c)
#else
# define debug(a)
# define debug1(a, b)
# define debug2(a, b, c)
#endif

#define USE_PIXMAP_IF_COLOR 1

#define DEFAULT_PLOCKDIR	"/local/X11R5/lib/plock"

#define DEFAULT_PLOCK_SOUNDS    "sound"
#define SND_OHNO                "oh_no.snd"
#define SND_EXPLODE             "explode.snd"
#define SND_BOXOPEN             "open_the_box.snd"
#define SND_LETSGO              "lets_go.snd"
#define SND_NEWLEVEL            "new_level.snd"


#ifdef NOCATFILES
# define DEFAULT_PLOCK_MASKS      "mask"
# define DEFAULT_PLOCK_BW_FRAMES  "rasbw"
# define DEFAULT_PLOCK_COL_FRAMES "ras"
#else
# define DEFAULT_PLOCK_MASKS      "catmask"
# define DEFAULT_PLOCK_BW_FRAMES  "catrasbw"
# define DEFAULT_PLOCK_COL_FRAMES "catras"
#endif

/*
 * approximate speed_factor for various architectures: 
 *    SS2	250..325
 *    SLC       490..625
 *    sun3     1500..1675
 *    hp300    2500..2600
 *
 * the number of lemmings is calculated as
 *   maxlems = 1 + LEMSPEED_BASE / speed_factor
 */
#define LEMSPEED_BASE	1800

#define TEXT_XP	100
#define TEXT_YP	100
#define TICK_SIZE_X 300
#define TICK_SIZE_Y 170
#define TICK_OFFSET_X (stage.screenx - stage.Clock->width - 100 + 33)
#define TICK_OFFSET_Y (stage.screeny -stage.Clock->height + 35)
#define LABEL_OFFSET_X (stage.screenx - stage.Clock->width - 100 + 125)
#define LABEL_OFFSET_Y (stage.screeny - stage.Clock->height + 242)

struct Option
{
  int speed; /* 2 for demo, 60, 120, 180 for 15, 30, 45 minutes timeout */
  int harmless;
  int color;
  int noanim;
  int nolock;
  int quiet;
  int nonoise;
  int vacation;
		int anon;
};
  
struct anim
{
  XImage **frames, **masks;
  int nframes, width, height, xstep, ystep;
  int refcount;
};

struct pathpoint
{
  int x, y;
  struct pathpoint *next;
};

struct movement
{
  struct anim *an;
  int prev_x, prev_y; 	/* where the previous frame was located */
  int pathx, pathy;	/* what the previous pathpoint was */
  int x, y;   		/* here we initially place our top left corner */
  int frame;		/* start with this frame */
  int step;		/* next frame is frame + step */
  int layer;		/* for collisions ... */
  int pixeltime;	/* milliseconds for a one pixel movement, or if 
  			 * an has zero x- and ystep, milliseconds to
  			 * the next frame */
  struct pathpoint *path_base;
  struct pathpoint *path;
};

#define LEM_NOTSTARTED	0
#define LEM_START	1
#define LEM_WALK1	2
#define LEM_FALL2	3
#define LEM_WALK2	4
#define LEM_BOMB	5
#define LEM_DEAD	6

struct lem_move
{
  int status, prev_status;
  int lifetime;
  struct movement *faller1, *walker1, *faller2, *walker2;
  struct movement *bomber, *fire, *numbers;
};


#define GRID_X_SHIFT	4
#define GRID_Y_SHIFT	5
#define GRID_KEEP	0
#define GRID_CLEAR	1
#define GRID_UPDATE	2
#define GRID_OFF(x, y) (((x) >> GRID_X_SHIFT) + ((y) >> GRID_Y_SHIFT) * (stage.grid_w))
#define GRID(x, y) stage.grid[GRID_OFF(x, y)]

#define MU_BLOCK	0
#define MU_POLL		1

struct _stage
{
  Display *Dis;
  Window Win;
  Visual *vis;
  int Sc;
  int islocal;
  unsigned long white, black, red; 
  GC bonw, wonb, bonwl;
  XImage *Back, *Work, *Clock, *Scale;
  int xmin, xmax, ymin, ymax;
  unsigned int depth, screenx, screeny;
  int grid_w, grid_h;
  char *grid;
  struct movement *vac_top, *vac_bot, *ticks, *box_top, *box_in, *box_open;
  int nlems;
  int release_rate;
  int beeper_volume;
  struct lem_move *lem;
};

extern time_t time();

extern struct _stage stage;
extern struct Option options;
extern char *LoginName;
extern void free_all __P((void));
extern int DrawFrameGrid __P((struct movement *, char *));
extern int EraseFrameGrid __P((struct movement *, char *));
extern int UpdateDisplayFromGrid __P((void));
extern int UpdateWorkFromBack __P((void));
extern int DestroyMovement __P((struct movement *));
extern int ImOverImGrid __P((XImage *, XImage *, XImage *, int, int, int, int, int, int, char *));
extern int ImOverIm __P((XImage *, XImage *, XImage *, int, int, int, int, int, int));
extern int ImFillRectangle __P((XImage *, int, int, int, int, int, char *));
extern int DrawLem __P((int));
extern int EraseLem __P((int));
extern int play_animation __P((void));
extern int play_explosion __P((int, int, int));
extern int SetGrid __P((char *, int, int, int, int, int));
extern int initstage __P((void));
extern void drawinit __P((void));
extern int initgraphics __P((void));
extern int mu __P((int, int));
extern XImage *XLoadRasterfile __P((Display *, Visual *, FILE *, colormap_t *, int));
extern int Image1to8 __P((XImage **, int));
extern int gettimeofday __P((struct timeval *, struct timezone *tzp));
extern int expl_init __P((int, int, int, int, int));
extern int explode __P((void));
extern int PlaySound __P((char *, int));
extern int WaitSoundPlayed __P((void));
extern int get_bell_vol __P((Display *));
extern void set_bell_vol __P((Display *, int));

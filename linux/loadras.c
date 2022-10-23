// from plock.c

#include <X11/Xlib.h>
Colormap xmap;

# define SEC_PER_TICK 60

int real_depth;

// from anim.c

#include "../plock.h"
#include "../config.h"

#define PADDING 32 /* bitmap_pad for XImage creation */

extern int real_depth;	// read access in anim.c - it is written in plock.c:main() 
struct _stage stage;	// stage.vis and stage.depth is used here.

struct Option options = 
{
  SEC_PER_TICK, 0, USE_COLOR, 0, 0, 0, 1, 0, 0
};
  
Colormap xmap, dmap;
char *dpyname, *LoginName = NULL;
Window rootwin;
Cursor crs;
static int status = 0, badlog;
static int hostnr, ssto, ssiv, ssbl, ssex, pipe_fd[2];
XHostAddress *hosts;
Bool hoststate;
unsigned char empty[2] = { 0, 0 };

static void set_colors(r, g, b, len)
  unsigned char *r, *g, *b;
  int len;
{
  extern Colormap xmap;
  XColor xcol;
  int i;

  for(i = 0; i < len; i++)
    {
      xcol.pixel = i;
      xcol.red = r[i] << 8;
      xcol.green = g[i] << 8;
      xcol.blue = b[i] << 8;
      xcol.flags = DoRed | DoGreen | DoBlue;
      XStoreColor(stage.Dis, xmap, &xcol);
    }
  /*
   * the red pixel
   */
  xcol.pixel = stage.red;
  xcol.red = 180 << 8;
  xcol.green = 50 << 8;
  xcol.blue = 50 << 8;
  xcol.flags = DoRed | DoGreen | DoBlue;
  XStoreColor(stage.Dis, xmap, &xcol);
}

XImage *
LoadImageFromRasterfileFp(Dis, Sc, fp)
Display *Dis;
int Sc;
FILE *fp;
{
  static int no_colors_yet = 1;
  colormap_t cmap;
  XImage *Im = NULL;

  cmap.map[0] = cmap.map[1] = cmap.map[2] = 0;
  Im = XLoadRasterfile(Dis, stage.vis, fp, &cmap, PADDING);

  if(Im == NULL)
    {
      perror("XLoadRasterfile");
      return NULL;
    }

  if(options.color && cmap.map[0] && no_colors_yet)
    {
      set_colors(cmap.map[0], cmap.map[1], cmap.map[2], cmap.length/3);
      no_colors_yet = 0;
    }
  if(cmap.map[0]) { free(cmap.map[0]); cmap.map[0] = 0; }
  if(cmap.map[1]) { free(cmap.map[1]); cmap.map[1] = 0; }
  if(cmap.map[2]) { free(cmap.map[2]); cmap.map[2] = 0; }

  if (cmap.length > (3 << stage.depth))
    {
      fprintf(stderr, "FILE *fp: picture depth(%d) > display depth(%d)\n",
	      cmap.length, stage.depth);
      return NULL;
    }
  return Im;
}

XImage *
LoadImageFromRasterfile(Dis, Sc, name)
Display *Dis;
int Sc;
char *name;
{
  XImage *Im = NULL;
  FILE *fp;
   
  if ((fp = fopen(name, "r")) == NULL)
    {
      perror(name);
      return NULL;
    }
  Im = LoadImageFromRasterfileFp(Dis, Sc, fp);
  fclose(fp);
  return Im;
}


int main(int argc, char **argv)
{
  char *dpyname = NULL;		// NULL: take env DISPLAY, or try ":0.0";
  XColor curcol;
  unsigned width, height;
  Window rootwin;

  // another excerpt from plock.c
  if((stage.Dis = XOpenDisplay(dpyname)) == NULL)
    {
      fprintf(stderr,
      "Can't connect server.\nPlease check the DISPLAY environment variable\n");
      exit(1);
    }
  stage.Sc = DefaultScreen(stage.Dis);
  real_depth = stage.depth = DefaultDepth(stage.Dis, stage.Sc);
  rootwin = RootWindow(stage.Dis, stage.Sc);
  width = DisplayWidth(stage.Dis, stage.Sc);
  height = DisplayHeight(stage.Dis, stage.Sc);
  stage.white = WhitePixel(stage.Dis, stage.Sc);
  stage.black = BlackPixel(stage.Dis, stage.Sc);

  bzero((char *)&curcol, sizeof(XColor));
  curcol.pixel = stage.black;
  /* "empty" cursor */

  printf("dummy main\n");
  return 0;
}

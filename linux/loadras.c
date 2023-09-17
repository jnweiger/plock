#include <assert.h>
#include <X11/Xutil.h>
#include <X11/keysym.h> // Include this for key symbols

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

  XImage *im = LoadImageFromRasterfile(stage.Dis, stage.Sc, argv[1]);
  if (im->depth == 1)
    Image1to8(&im, PADDING);	// huch, this converts to 24 bits per pixel.

  printf("dummy main, stage.depth = %d\n", real_depth);
  printf("Image size: w=%d h=%d, im->depth=%d, im->bits_per_pixel=%d\n", im->width, im->height, im->depth, im->bits_per_pixel);

  // Create a window
#define WIN_HEIGHT 600
#define WIN_WIDTH 800
  Window window = XCreateSimpleWindow(stage.Dis, rootwin, 0, 0, WIN_WIDTH, WIN_HEIGHT, 1, stage.black, stage.white);

  XSelectInput(stage.Dis, window, ExposureMask);
  XMapWindow(stage.Dis, window);
  // Wait for the MapNotify event
  XEvent event;
  do {
     XNextEvent(stage.Dis, &event);
  } while (event.type != MapNotify);

  // Create an X pixmap and draw the XImage onto it
  Pixmap pixmap = XCreatePixmap(stage.Dis, window, WIN_WIDTH, WIN_HEIGHT, real_depth);
  GC gc = DefaultGC(stage.Dis, stage.Sc);
  XPutImage(stage.Dis, pixmap, gc, im, 0, 0, 0, 0, im->width, im->height);

  // Draw the pixmap onto the window
  XCopyArea(stage.Dis, pixmap, window, gc, 0, 0, im->width, im->height, 0, 0);

  // Flush the display to ensure changes are visible
  XFlush(stage.Dis);

  // Event loop to handle window events (e.g., expose events)
  while (1)
    {
      XNextEvent(stage.Dis, &event);
      if (event.type == Expose)
        {
          // Redraw the pixmap on expose events
          XCopyArea(stage.Dis, pixmap, window, gc, 0, 0, im->width, im->height, 0, 0);
          XFlush(stage.Dis);
        }
      else if (event.type == KeyPress)
	{
	  KeySym keySym = XLookupKeysym(&event.xkey, 0);
	  if (keySym == XK_q)	// 'q' exits the loop.
	    break;
	}
    }

  // Clean up
  XDestroyImage(im);
  XFreePixmap(stage.Dis, pixmap);
  XCloseDisplay(stage.Dis);

  return 0;
}

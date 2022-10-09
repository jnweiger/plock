/*
 * anim.c -- plock animation code
 *
 * jw 14.1.1992
 */
#include "rcs.h"
RCS_ID("$Id$ FAU");


#include "plock.h"

#define PADDING 32 /* bitmap_pad for XImage creation */
#define GAIN 40	   /* volume of sparc /dev/audio */

#ifndef index
extern char *index();
#endif
extern int real_depth;
struct _stage stage;

static int mu_drawframe __P((int, int));

/*
 * set the environment variables
 * PLOCK_MASKS, and
 * PLOCK_COL_FRAMES, PLOCK_BW_FRAMES or PLOCK_FRAMES
 * to modify the search directories.
 */
static char *plockdir, *maskdir, *framedir, *sounddir;

#if 0
int rotate()
{
  static char r[] = "\\|/-";
  static int p = 0;

  printf("%c\r", r[p]);
  fflush(stdout);
#if 1
  chkmem();
#endif
  if (++p > 3)
    p = 0;
}
#endif

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
  static no_colors_yet = 1;
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

#if 0
XImage *
LoadImageFromRasterfile(Dis, Sc, name)
Display *Dis;
int Sc;
char *name;
{
  XImage *Im = NULL;
  FILE *fp;
  Pixrect *Pr;
  colormap_t cmap;
  unsigned char *data;

  if ((fp = fopen(name, "r")) == NULL)
    {
      perror(name);
      return NULL;
    }
  if ((Pr = pr_load(fp, &cmap)) == NULL)
    {
      fprintf(stderr, "pr_load '%s' failed\n", name);
      fclose(fp);
      return NULL;
    }
  fclose(fp);
  if (cmap.length > (1 << stage.depth) || (cmap.length == 0 && stage.depth > 1))
    {
      fprintf(stderr, "%s: picture depth(%d) != display depth(%d)\n",
	      name, cmap.length, stage.depth);
      pr_destroy(Pr);
      fclose(fp);
      return NULL;
    }
  data = (unsigned char *)((struct mpr_data *)(Pr->pr_data))->md_image;
  (caddr_t)((struct mpr_data *)(Pr->pr_data))->md_image = NULL;
  pr_destroy(Pr);
  Im = XCreateImage(Dis, stage.vis, stage.depth,
                    stage.depth == 1 ? XYPixmap : ZPixmap, 0, (char *)data, 
                    Pr->pr_size.x, Pr->pr_size.y, 16, 0);
  if (Im->depth > 1)
    {
      fprintf(stderr, "sorry colormap conversion not done\n");
    }
  return Im;
}
#endif

#if 0
void
AdjustImage(im)
XImage *im;
{
/* Reverse the bits for some funny displays... */
  if (im->depth == 1 && !stage.black)
    {
      int x, y, bpl = im->bytes_per_line;
      unsigned char *dp;

      dp = (unsigned char *)im->data;
      for (y = 0; y < im->height; y++)
        {
          for (x = 0; x < bpl; x++)
            dp[x] = ~dp[x];
          dp += bpl;
        }
    }
}
#endif
 
/* 
#ifdef NOCATFILES
 * name is a concatenated archive of frames
#else
 * name should be something like "box%04d.ras"
 * it is used in 'sprintf(buf, name, frame + offset)'.
 * same applies to mask, if present.
#endif
 * framedir and maskdir are referenced...
 */
struct anim *
LoadAnimFromRasterfiles(Dis, Sc, name, mask, nframes, offset, xstep, ystep)
Display *Dis;
int Sc;
char *name, *mask;
int nframes, offset, xstep, ystep;
{
  static char buf[MAXPATHLEN];
#ifdef NOCATFILES
  static char fmt[MAXPATHLEN];
#endif
  struct anim *An;
  XImage **Imp;
  int black, i;
  FILE *fp;
	 
  An = (struct anim *)malloc(sizeof(struct anim));
  An->frames = (XImage **)malloc(sizeof(XImage *) * nframes);
  if (mask)
    An->masks = (XImage **)malloc(sizeof(XImage *) * nframes);
  else
    An->masks = NULL;
  An->nframes = nframes;
  An->xstep = xstep;
  An->ystep = ystep;
  An->refcount = 0;
  Imp = An->frames;
  black = stage.black;
#ifndef NOCATFILES
  sprintf(buf, "%s/%s/%s", plockdir, framedir, name);
  if ((fp = fopen(buf, "r")) == NULL)
    {
      Free(An);
      fprintf(stderr, "cannot read '%s'\n", buf);
      return NULL;
    }
#endif
  for (i = 0; i < nframes; i++)
    {
#ifdef NOCATFILES
      sprintf(fmt, "%s/%s/%s", plockdir, framedir, name);
      sprintf(buf, fmt, i + offset);
      if ((*Imp = LoadImageFromRasterfile(Dis, Sc, buf)) == NULL)
#else
      if ((*Imp = LoadImageFromRasterfileFp(Dis, Sc, fp)) == NULL)
#endif
        {
          fprintf(stderr, "cannot LoadImageFromRasterfile(Dis, Sc, '%s');\n",
                  buf);
	  while (i-- > 0)
	    {
	      Imp--;
	      XDestroyImage(*Imp);
	    }
          Free(An);
          return NULL;
        }
      if (stage.depth > 1 && options.color == 0)
	{
	  Image1to8(Imp, PADDING);
	}
      if ((*Imp)->depth != stage.depth)
	{
	  fprintf(stderr, "%s: depth %d does not match display depth %d\n",
		  name, (*Imp)->depth, stage.depth);
	  while (i-- > 0)
	    XDestroyImage(*--Imp);
	  Free(An);
	  return NULL;
	}
      if (i == 0)
        {
          An->width = (*Imp)->width;
          An->height = (*Imp)->height;
        }
      else
        {
          if (An->width != (*Imp)->width || An->height != (*Imp)->height)
            {
              fprintf(stderr, 
"frame size mismatch: %s (%d, %d) should match size of first frame (%d, %d)\n",
                      buf, (*Imp)->width, (*Imp)->height, 
                      An->width, An->height);
              Free(An);
              return NULL;
            }
	}
      if (real_depth == 1 && !stage.black)
        {
	  /* we have to invert the image */
	  char *p = (*Imp)->data;
	  int l = (*Imp)->bytes_per_line * (*Imp)->height;

	  while (l--)
	    {
	      *p = ~(*p);
	      p++;
	    }
	}
      Imp++;
    }
#ifndef NOCATFILES
  fclose(fp);
#endif
  if (mask)  
    {
      char *p;
      int l;

      Imp = An->masks;
#ifndef NOCATFILES
      sprintf(buf, "%s/%s/%s", plockdir, maskdir, mask);
      if ((fp = fopen(buf, "r")) == NULL)
	{
	  fprintf(stderr, "cannot read '%s'\n", buf);
	  for (i = 0; i < nframes; i++)
	    XDestroyImage(An->frames[i]);
	  Free(An);
	  return NULL;
	}
#endif

      for (i = 0; i < nframes; i++)
        {
#ifdef NOCATFILES
          sprintf(fmt, "%s/%s/%s", plockdir, maskdir, mask);
          sprintf(buf, fmt, i + offset);
	  if ((*Imp = LoadImageFromRasterfile(Dis, Sc, buf)) == NULL) 
#else
	  if ((*Imp = LoadImageFromRasterfileFp(Dis, Sc, fp)) == NULL)
#endif
            {
              fprintf(stderr, "cannot LoadImageFromRasterfile(Dis,Sc,'%s');\n",
                      buf);
	      while (i-- > 0)
		XDestroyImage(*--Imp);
	      for (i = 0; i < nframes; i++)
		XDestroyImage(An->frames[i]);
              Free(An);
              return NULL;
            }
	  if ((*Imp)->depth != 1)
	    {
	      fprintf(stderr, "mask %s: depth %d ???\n", buf, (*Imp)->depth);
	      while (i-- > 0)
		XDestroyImage(*--Imp);
	      for (i = 0; i < nframes; i++)
		XDestroyImage(An->frames[i]);
	      Free(An);
	      return NULL;
	    }
          if (An->width > (*Imp)->width || An->height > (*Imp)->height)
            {
              fprintf(stderr, 
"mask size mismatch: %s (%d, %d) should match size of first frame (%d, %d)\n",
                      buf, (*Imp)->width, (*Imp)->height, 
                      An->width, An->height);
              Free(An);
              return NULL;
            }
	  p = (*Imp)->data;
	  l = (*Imp)->bytes_per_line * (*Imp)->height;
	  while (l--)
	    {
	      *p = ~(*p);
	      p++;
	    }
	  Imp++;
        } 
#ifndef NOCATFILES
      fclose(fp);
#endif
    }
  return An;
}

int DestroyAnim(an)
struct anim *an;
{
  int i;
  
  if (an->refcount > 0)
    {
      return 1;
    }
  for (i = 0; i < an->nframes; i++)
    {
      if (an->frames && an->frames[i])
	XDestroyImage(an->frames[i]);
      if (an->masks && an->masks[i])
	XDestroyImage(an->masks[i]);
    }
  Free(an);
  return 0;
}

int
skipspace(sp)
char **sp;
{
  char *s;

  if (!sp)
    return 1;
  s = *sp;
  while (*s == ' ' || *s == '\t')
    s++;
  if (s == *sp)
    return 1;
  *sp = s;
  return 0;
}

int 
skipuntil(s, sp)
char *s, **sp;
{
  char *t, *p;

  if (!s || !sp)
    return 1;
  p = *sp;
  while(*p)
    {
      for (t = s; *t; t++)
	if (*t == *p)
	  {
	    if (p == *sp)
	      return 1;
	    *sp = p;
	    return 0;
	  }
      p++;
    }
  *sp = p;
  return 2;
}

int
skipnum(pp, ip)
char **pp;
int *ip;
{
  int r = 0;
  int sig = 1;
  
  if (!pp || !*pp)
    return r;
  if (ip)
    *ip = 0;
  skipspace(pp);
  if (**pp == '-')
    {
      sig = -1;
      (*pp)++;
    }
  while(**pp)
    {
      if (**pp >= '0' && **pp <= '9')
        {
          if (ip)
            *ip = *ip * 10 + **pp - '0';
          r = 1;
        }
      else
        break;
      (*pp)++;
    }
  skipspace(pp);
  *ip *= sig;
  return r;
}      
  
/*
 * path is a komma seperated list of decimal number pairs
 */
struct movement *
CreateMovementFromAnim(An, x, y, startframe, framestep, pixtime, layer, path)
struct anim *An;
int layer, x, y, startframe, framestep, pixtime;
char *path;
{
  struct movement *Mv;
  struct pathpoint **pp;
  int x1, y1;
  
  Mv = (struct movement *)malloc(sizeof(struct movement));
  Mv->an = An;
  An->refcount++;
  Mv->x = Mv->pathx = x;
  Mv->y = Mv->pathy = y;
  Mv->frame = startframe % An->nframes; 
  Mv->step = framestep;
  Mv->pixeltime = pixtime;
  Mv->layer = layer;
  pp = &Mv->path_base;
    
  while (skipnum(&path, &x1) && skipnum(&path, &y1))
    {
      if (*path == ',')
        path++;
      *pp = (struct pathpoint *)malloc(sizeof(struct pathpoint));
      (*pp)->x = x1;
      (*pp)->y = y1;
      pp = &(*pp)->next;
    }
  *pp = NULL;
  Mv->path = Mv->path_base;
  return Mv;
}   

int 
DestroyMovement(mv)
struct movement *mv;
{
  if (--mv->an->refcount <= 0)
    DestroyAnim(mv->an);
  while (mv->path_base)
    {
      mv->path = mv->path_base->next;
      Free(mv->path_base);
      mv->path_base = mv->path;
    }
  Free(mv);
  return 0;
}
						
int 
SetGrid(g, x1, y1, x2, y2, val)
char *g;
int x1, y1, x2, y2, val;
{
  int i, j, k;

  /*
  printf("SetGrid(%d, %d, %d, %d,  %d);\n", x1, y1, x2, y2, val);
  */
  k = stage.grid_w;
  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;
  g += GRID_OFF(x1, y1);
  x1 >>= GRID_X_SHIFT;
  x2 >>= GRID_X_SHIFT;
  y1 >>= GRID_Y_SHIFT;
  y2 >>= GRID_Y_SHIFT;
  if (x2 >= k)
    x2 = k - 1;
  if (y2 >= stage.grid_h)
    y2 = stage.grid_h - 1;
  x2 -= x1;
  for (j = y1; j <= y2; j++)
    {
      for (i = 0; i <= x2; i++)
        g[i] = (char) val;
      g += k;
    }
  return 0;
}

int 
EraseFrameGrid(mp, g)
struct movement *mp;
char *g;
{
  return SetGrid(g, mp->prev_x, mp->prev_y, 
		 mp->prev_x + mp->an->width,
		 mp->prev_y + mp->an->height, GRID_CLEAR);
}
  
int
frame_adjust(mp, x, y)
struct movement *mp;
int x, y;
{
  struct pathpoint *p;

  mp->prev_x += x;
  mp->prev_y += y;
  mp->pathx += x;
  mp->pathy += y;
  mp->x += x;
  mp->y += y;
  p = mp->path;
  while (p)
    {
      p->x += x;
      p->y += y;
      p = p->next;
    }
  return 0;
}

int
DrawFrameGrid(mp, g)
struct movement *mp;
char *g;
{
  XImage *Im, *Imm;
  int pathend, flag, dx, dy;

  if (!mp || !mp->an || !mp->an->frames)
    {
      fprintf(stderr, "DrawFrameGrid: no move, no anim, no frames, no luck\n");
      return -1;
    }
  /* 
   * Hack: numbers come here with frame == -1, as we do not have a zero.
   */
  if (mp->frame < 0)
    return 0;
  Im = mp->an->frames[mp->frame];
  Imm = mp->an->masks ? mp->an->masks[mp->frame] : NULL;
  ImOverImGrid(stage.Work, Im, Imm, mp->x, mp->y, 
	       0, 0, Im->width, Im->height, g);
  mp->prev_x = mp->x;
  mp->prev_y = mp->y;
  mp->frame += mp->step;
  flag = pathend = 0;
  while (mp->frame < 0) 
    {
      mp->frame += mp->an->nframes;
      flag = 1;
    }
  while (mp->frame >= mp->an->nframes) 
    {
      mp->frame -= mp->an->nframes;
      flag = 1;
    }
  dx = dy = 0;
  if (mp->path)
    {
      dx = mp->path->x - mp->pathx;
      dy = mp->path->y - mp->pathy;
      if (mp->an->xstep)
        {
          if (!dx)
	    {
	      mp->x = mp->path->x;
	      mp->y = mp->path->y;
	      pathend = 1;
	    }
          else
	    {
	      mp->x += mp->an->xstep;
	      mp->y = mp->pathy + (dy * (mp->x - mp->pathx)) / dx;
	    }
          if ((mp->x - mp->an->xstep > mp->path->x) != (mp->x > mp->path->x))
	    pathend = 1;
        }
      else
        {
	  /* !mp->an->xstep */
          if (mp->an->ystep)
            {
              if (!dy)
	        {
	          mp->x = mp->path->x;
	          mp->y = mp->path->y;
	          pathend = 1;
	        }
              else
	        {
	          mp->y += mp->an->ystep;
	          mp->x = mp->pathx + (dx * (mp->y - mp->pathy)) / dy;
	        }
              if ((mp->y - mp->an->ystep > mp->path->y) != (mp->y > mp->path->y))
	        pathend = 1;
	    }
          else
	    {
	      /* !mp->an->xstep && !mp->an->ystep */
	      mp->x = mp->path->x;
	      mp->y = mp->path->y;
	      pathend = 1;
	    }
        }
      if (pathend)
	{
	  /* we reached our current pathpoint */
	  mp->pathx = mp->path->x;
	  mp->pathy = mp->path->y;
	  mp->path = mp->path->next;
	}
    }
  else
    {
      /* !mp->path */
      mp->y += mp->an->ystep;
      mp->x += mp->an->xstep;
    }
  if ((flag || pathend) && !mp->path)
    /* our anim did a wrap around, or we had a path that ended here */
    return 1;
  return 0;
}

#ifdef NOCATFILES
# define CLOCK_FILE 	"clock.ras"
# define SCALE_FILE	"scale%d.ras"
# define TICKS_FILES 	"ticks%d.ras"
# define TICKSM_FILES 	"ticks%d.ras"
# define BOX_FILES 	"box%04d.ras"
# define BOXM_FILES 	"box%04d.ras"
# define FALL_FILES	"fall%04d.ras"
# define FALLM_FILES	"fall%04d.ras"
# define WALK_FILES	"walk%04d.ras"
# define WALKM_FILES	"walk%04d.ras"
# define BOMB_FILES	"bomb%04d.ras"
# define BOMBM_FILES	"bomb%04d.ras"
# define FIRE_FILES	"fire%04d.ras"
# define FIREM_FILES	"fire%04d.ras"
# define NUM_FILES	"letter%d.ras"
# define NUMM_FILES	"letter%d.ras"
# define VAC_FILE	"vacation.ras"
# define VACM_FILE	"vacation.ras"
# define VACP_FILE	"vac_pole.ras"
# define VACPM_FILE	"vac_pole.ras"
#else
# define CLOCK_FILE 	"clock.ras"
# define SCALE_FILE	"scale%d.ras"
# define TICKS_FILES 	"ticks.ras"
# define TICKSM_FILES 	"ticks.ras"
# define BOX_FILES 	"boxes.ras"
# define BOXM_FILES 	"boxes.ras"
# define FALL_FILES	"fallers.ras"
# define FALLM_FILES	"fallers.ras"
# define WALK_FILES	"walkers.ras"
# define WALKM_FILES	"walkers.ras"
# define BOMB_FILES	"bombers.ras"
# define BOMBM_FILES	"bombers.ras"
# define FIRE_FILES	"fires.ras"
# define FIREM_FILES	"fires.ras"
# define NUM_FILES	"letters.ras"
# define NUMM_FILES	"letters.ras"
# define VAC_FILE	"vacation.ras"
# define VACM_FILE	"vacation.ras"
# define VACP_FILE	"vac_pole.ras"
# define VACPM_FILE	"vac_pole.ras"
#endif

int
init_lem_move(lp)
struct lem_move *lp;
{
  struct anim *An;
  int x, y;
  char path[MAXPATHLEN];
  Display *Dis = stage.Dis;
  int Sc = stage.Sc;

  lp->status = lp->prev_status = LEM_NOTSTARTED;
  /* 
   * the faller1 comes out of the box, he appears at the bottom edge of
   * box_top. He falls down, until he lands on top of the clock.
   */
  if (stage.lem->faller1)
    An = stage.lem->faller1->an;
  else
    An = LoadAnimFromRasterfiles(Dis, Sc, FALL_FILES, FALLM_FILES, 8, 1, 0, 12);
  if (!An)
    return 1;
  x = 10 + stage.box_open->x + (stage.box_open->an->width >> 1) - (An->width >> 1);
  y = 11 + stage.screeny - stage.Clock->height - An->height;
  sprintf(path, "%d %d", x, y + 34);
  lp->faller1 = CreateMovementFromAnim(An, 
  			x, stage.box_top->an->height - An->height, 
  			0, 1, 100, 1, path);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /*
   * walker1 walks from center of clock to left edge.
   */
  if (stage.lem->walker1)
    An = stage.lem->walker1->an;
  else
    An = LoadAnimFromRasterfiles(Dis, Sc, WALK_FILES, WALKM_FILES, 16, 1, -4, 0);
  if (!An)
    return 1;
  x = 10 + stage.screenx - stage.Clock->width - 100 - An->width;
  sprintf(path, "%d %d, %d %d, %d %d, %d %d",
	  x + 246, y + 6, x + 140, y + 6, x + 56, y + 34, x + 16, y + 80);
  lp->walker1 = CreateMovementFromAnim(An, lp->faller1->x + 30, y + 30,
  			0, 1, 200, 0, path);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /* 
   * faller2 drops off the left edge of the clock and hits the floor.
   */
  sprintf(path, "%d %d", x - 5, stage.screeny - An->height - 10);
  lp->faller2 = CreateMovementFromAnim(lp->faller1->an, 
			x - 5, y + 88,
 			0, 1, 100, 0, path);
  /* 
   * walker2 walks on the floor off the stage, left hand side.
   */
  y = stage.screeny - An->height;
  sprintf(path, "%d %d", -An->width * 2, y);
  lp->walker2 = CreateMovementFromAnim(lp->walker1->an, x + 30, y, 
  			0, 1, 200, 0, path);
  /*
   * bomber comes back from the left side and walks to the middle of the 
   * screen. He won't make it to the other side...
   */
  if (stage.lem->bomber)
    An = stage.lem->bomber->an;
  else
    An = LoadAnimFromRasterfiles(Dis, Sc, BOMB_FILES, BOMBM_FILES, 16, 1, 4, 0);
  if (!An)
    return 1;
  y = stage.screeny - An->height;
  lp->bomber = CreateMovementFromAnim(An, -An->width, y - 8, 0, 1, 300, 0, NULL);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /*
   * the fire burns down to the bomb, as it is carried rightward.
   */
  if (stage.lem->fire)
    An = stage.lem->fire->an;
  else
  An = LoadAnimFromRasterfiles(Dis, Sc, FIRE_FILES, FIREM_FILES, 16, 1, 4, 0);
  if (!An)
    return 1;
  lp->fire = CreateMovementFromAnim(An, lp->bomber->x + 66, y + 13 - An->height,
			0, 1, 300, 1, NULL);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /* 
   * numbers are strange. the don't flip on every frame. but they move. 
   */
  if (stage.lem->numbers)
    An = stage.lem->numbers->an;
  else
    An = LoadAnimFromRasterfiles(Dis, Sc, NUM_FILES, NUMM_FILES, 5, 1, 4, 0);
  if (!An)
    return 1;
  lp->numbers = CreateMovementFromAnim(An,
			lp->bomber->x + 20, y - An->height - 30,
			4, 0, 300, 2, NULL);
  mu_drawframe(options.speed * 1000, MU_POLL);
  return 0;
}
  
int 
initstage()
{
  Window Win;
  Display *Dis;
  int Sc;
  int i;
  XImage *Im;
  Window dwin;
  struct anim *An;
  char *w, *b;
  unsigned int ii;
  char buf[MAXPATHLEN];

  Win = stage.Win;
  Dis = stage.Dis;
  Sc = stage.Sc;
  XGetGeometry(Dis, DefaultRootWindow(Dis), &dwin,
       &i, &i, &stage.screenx, &stage.screeny, &ii, &ii);
  stage.grid_w = ((stage.screenx - 1) >> GRID_X_SHIFT) + 1;
  stage.grid_h = ((stage.screeny - 1) >> GRID_Y_SHIFT) + 2;
  stage.grid = (char *)malloc(stage.grid_w * stage.grid_h);
  Im = XCreateImage(Dis, stage.vis, stage.depth,
                    stage.depth == 1 ? XYPixmap : ZPixmap, 0, NULL, 
                    stage.screenx, stage.screeny, PADDING, 0);
  Im->data = (char *)calloc(Im->bytes_per_line, Im->height);
  stage.Back = Im;
  Im = (XImage *)malloc(sizeof(XImage));
  bcopy((char *)stage.Back, (char *)Im, sizeof(XImage));
  Im->data = (char *)calloc(Im->bytes_per_line, Im->height);
  stage.Work = Im;
  w = stage.Work->data;
  b = stage.Back->data;
  ii = stage.depth == 1 ? 0xff : stage.black;
  for (i = stage.Work->bytes_per_line * stage.Work->height; i > 0; i--)
    {
      *w++ = *b++ = ii;
    }
  
  /*
   * now we have to locate the image files:
   */

  if ((plockdir = getenv("PLOCKDIR")) == NULL)
    plockdir = DEFAULT_PLOCKDIR;
  if ((sounddir = getenv("PLOCK_SOUNDS")) == NULL)
    sounddir = DEFAULT_PLOCK_SOUNDS;
  if ((maskdir = getenv("PLOCK_MASKS")) == NULL)
    maskdir = DEFAULT_PLOCK_MASKS;
  if (stage.depth == 8 && options.color)
    framedir = getenv("PLOCK_COLOR_FRAMES");
  else
    framedir = getenv("PLOCK_BW_FRAMES");
  if (!framedir && (framedir = getenv("PLOCK_FRAMES")) == NULL)
    {
      if (stage.depth == 8 && options.color)
        framedir = DEFAULT_PLOCK_COL_FRAMES;
      else
        framedir = DEFAULT_PLOCK_BW_FRAMES;
    }
  sprintf(buf, "%s/%s/%s", plockdir, framedir, CLOCK_FILE);
  if ((Im = LoadImageFromRasterfile(Dis, Sc, buf)) == NULL)
    {
      fprintf(stderr, "plock: Cannot load %s, HELP!\n", buf);
      return 1;
    }
  if (real_depth == 1 && !stage.black)
    {
      /* we have to invert the clock */
      char *p = Im->data;
      int l = Im->bytes_per_line * Im->height;

      while (l--)
	{
	  *p = ~(*p);
	  p++;
	}
    }
  if (stage.depth == 8 && options.color == 0)
    Image1to8(&Im, PADDING);
  stage.Clock = Im;
  {
    char p[256];
    int i = 45;

    if (options.speed < 180)
      i = 30;
    if (options.speed < 120)
      i = 15;

    sprintf(p, SCALE_FILE, i);
    sprintf(buf, "%s/%s/%s", plockdir, framedir, p);
  }
  if ((Im = LoadImageFromRasterfile(Dis, Sc, buf)) == NULL)
    {
      fprintf(stderr, "plock: Cannot load %s, HELP!\n", buf);
      return 1;
    }
  if (stage.depth == 8 && options.color == 0)
    Image1to8(&Im, PADDING);
  stage.Scale = Im;
  stage.xmin = stage.xmax = stage.ymin = stage.ymax = 0;
  
  /*
   * the ticks are stationary on the face of the clock 
   * we load all.
   */
  
  An = LoadAnimFromRasterfiles(Dis, Sc, TICKS_FILES, TICKSM_FILES, 21, -5, 0, 0);
  if (!An)
    {
      fprintf(stderr, "plock: Well, %s found, but where are %s?\n",
	      CLOCK_FILE, TICKS_FILES);
      return 1;
    }
  stage.ticks = CreateMovementFromAnim(An, 
  			stage.screenx - stage.Clock->width - 100 + 36,
  			stage.screeny - stage.Clock->height + 35,
  			20, -1, options.speed * 1000, 0, NULL);
  return 0;
}

static int mu_drawframe(to, poll)
int to, poll;
{
  static int end = 0;

  if (!end && mu(to, poll))
    {
      end = DrawFrameGrid(stage.ticks, stage.grid);
      UpdateDisplayFromGrid();
      EraseFrameGrid(stage.ticks, stage.grid);
      UpdateWorkFromBack();
    }
  return end;
}
      
int initgraphics()
{
  Display *Dis;
  int Sc;
  int r, i, x;
  struct anim *An;
  char path[MAXPATHLEN];
  struct lem_move *lp;

  Dis = stage.Dis;
  Sc = stage.Sc;

  /* 
   * we need to keep the XImage structure itself, see below
   */
  if (stage.Clock->data)
    Free(stage.Clock->data);
  /*
   * the ticks are stationary on the face of the clock 
   * Ticks are already loaded.
   */
  mu_drawframe(options.speed * 1000, MU_POLL);
  if (options.noanim)
    {
      stage.box_top = NULL;
      stage.box_in = NULL;
      stage.box_open = NULL;
      stage.nlems = 0;
      return 0;
    }
  /*
   * We have an alternate (simple story) if gaffers call
   * plock just to see our lemmings.
   */
  if (options.vacation)
    {
      An = LoadAnimFromRasterfiles(Dis, Sc, VACP_FILE, VACPM_FILE, 1, 1, 0, 20);
      if (!An)
	return 1;
      x = stage.screenx - (stage.Clock->width >> 1) - (An->width >> 1);
      sprintf(path, "%d %d", x, 
	      stage.screeny - stage.Clock->height - An->height + 50);
      stage.vac_bot = CreateMovementFromAnim(An, x, -(An->height / 4 * 3),
			 0, 0, 100, 0, path);
      mu_drawframe(options.speed * 1000, MU_POLL);
      An = LoadAnimFromRasterfiles(Dis, Sc, VAC_FILE, VACM_FILE, 1, 1, 0, 16);
      if (!An)
	return 1;
      x = stage.screenx - (stage.Clock->width >> 1) - (An->width >> 1);
      sprintf(path, "%d %d", x, stage.screeny - An->height - stage.vac_bot->an->height);
      stage.vac_top = CreateMovementFromAnim(An, x, -An->height, 0, 0, 
			100, 0, path);
      mu_drawframe(options.speed * 1000, MU_POLL);

      /*
       * {-: we need no box, no lemmings and hope that nobody noticed that 
       * loading was finished so quickly :-}
       */
      return 0;
    }
  /*
   * box_top is a stationary upper half of the lemming box 
   * placed at the top edge of the screen above the clock.
   */
  An = LoadAnimFromRasterfiles(Dis, Sc, BOX_FILES, NULL, 1, 1, 0, 0);
  if (!An)
    return 1;
  An->frames[0]->height = An->height = 25;
  stage.box_top = CreateMovementFromAnim(An, 
  			stage.screenx - (stage.Clock->width >> 1)
			- (An->width >> 1),
  			0, 0, 0, 0, 2, NULL);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /*
   * box_in is the still closed box moving downward to appear over the clock.
   */
  An = LoadAnimFromRasterfiles(Dis, Sc, BOX_FILES, NULL, 1, 1, 0, 2);
  if (!An)
    return 1;
  sprintf(path, "%d %d", stage.box_top->x, stage.box_top->y);
  stage.box_in = CreateMovementFromAnim(An, 
  			stage.box_top->x, -An->height >> 1, 
  			0, 0, 100, 0, path);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /*
   * box_open is the box opening its doors (stationary).
   */
  An = LoadAnimFromRasterfiles(Dis, Sc, BOX_FILES, BOXM_FILES, 16, 1, 0, 0);
  if (!An)
    return 1;
  stage.box_open = CreateMovementFromAnim(An, 
  			stage.box_top->x, stage.box_top->y,
  			0, 1, 200, 0, NULL);
  mu_drawframe(options.speed * 1000, MU_POLL);
  /*
   * we do the setup for one lemming only
   */
  stage.nlems = 12;
  stage.release_rate = 30;
  stage.lem = (struct lem_move *)calloc(stage.nlems, sizeof(struct lem_move));
  lp = stage.lem;
  for (i = 0; i < stage.nlems; i++)
    {
      r = init_lem_move(lp);
      if (r)
	{ 
	  stage.nlems = i;
	  return 1;
	}
      lp++;
    }
  return 0;
}

static char *labeltext[] =
{
  " ",
  "IMMD",
  "CIP Parkplatz",
  " ",
  "Mo. - So.  0h - 24h",
  "H\366chstparkdauer:",
  NULL,
};

int dumpgrid()
{
  int i, j;
  char *g;

  g = stage.grid;
  for (j = 0; j < stage.grid_h; j++)
    {
      for (i = 0; i < stage.grid_w; i++)
	{
	  switch (*g)
	    {
	    case GRID_CLEAR:
	      printf("O ");
	      break;
	    case GRID_UPDATE:
	      printf("X ");
	      break;
	    default:
	      printf(". ");
	      break;
	    }
	  g++;
	}
      printf("\n");
    }
  return 0;
}

void see(im)
  XImage *im;
{
#if USE_PIXMAP_IF_COLOR
  static Pixmap pm = 0;
  static GC pgc;

  if(pm == 0)
    {
      pm = XCreatePixmap(stage.Dis, stage.Win, 
					     stage.screenx, stage.screeny, 1);
      pgc = XCreateGC(stage.Dis, pm, 0, 0);
    }
  XPutImage(stage.Dis, pm, pgc, im, 
                                       0,0,0,0, stage.screenx, stage.screeny);
  XCopyPlane(stage.Dis, pm,  stage.Win, stage.bonw,
				     0,0, stage.screenx, stage.screeny, 0,0, 1);
#else
  XPutImage(stage.Dis, stage.Win, stage.bonw, im,
                                       0,0,0,0, stage.screenx, stage.screeny);
#endif
  XFlush(stage.Dis);
}


int 
UpdateWorkFromBack()
{
  int i, j, k = 0, w;
  char *g, *p;

  g = stage.grid;
  for (j = 0; j < stage.grid_h; j++)
    {
      for (i = 0; i < stage.grid_w;)
	{
	  switch (*g)
	    {
	    case GRID_CLEAR:
	      for (k = i + 1, p = g + 1; k < stage.grid_w; k++, p++)
		{
		  if (*p != *g)
		    break;
		  else 
		    *p = GRID_UPDATE;
		}
	      *g = GRID_UPDATE;
	      w = k - i;
	      ImOverIm(stage.Work, stage.Back, NULL, 
		       i << GRID_X_SHIFT, j << GRID_Y_SHIFT,
		       i << GRID_X_SHIFT, j << GRID_Y_SHIFT,
		       w << GRID_X_SHIFT, 1 << GRID_Y_SHIFT);
	      g += w;
	      i += w;
	      break;
	    default:
	      i++;
	      g++;
	      break;
	    }
	}
    }
  return 0;
}

int
UpdateDisplayFromGrid()
{
  int i, j, k = 0, w;
  char *g, *p;
#if USE_PIXMAP_IF_COLOR
  static Pixmap pm = 0;
  static GC pgc;
#endif

  g = stage.grid;
  for (j = 0; j < stage.grid_h; j++)
    {
      for (i = 0; i < stage.grid_w;)
	{
	  switch (*g)
	    {
	    case GRID_CLEAR:
	      for (k = i + 1, p = g + 1; k < stage.grid_w; k++, p++)
		{
		  if (*p != *g)
		    break;
		}
	      w = k - i;
	      ImOverIm(stage.Work, stage.Back, NULL, 
		       i << GRID_X_SHIFT, j << GRID_Y_SHIFT,
		       i << GRID_X_SHIFT, j << GRID_Y_SHIFT,
		       w << GRID_X_SHIFT, 1 << GRID_Y_SHIFT);
	      /* FALLTHROUGH */
	    case GRID_UPDATE:
	      for (k = i + 1, p = g + 1; k < stage.grid_w; k++, p++)
		{
		  if (*p != *g)
		    break;
		  else 
		    *p = GRID_KEEP;
		}
	      *g = GRID_KEEP;
	      w = k - i;
#if USE_PIXMAP_IF_COLOR
              if(real_depth != stage.depth)
		{
		  if(pm == 0)
		    {
		      pm = XCreatePixmap(stage.Dis, stage.Win, 
					 stage.screenx, stage.screeny, 1);
		      pgc = XCreateGC(stage.Dis, pm, 0, 0);
		    }
		  XPutImage(stage.Dis, pm, pgc, stage.Work, 
		      i << GRID_X_SHIFT, j << GRID_Y_SHIFT, 
		      0, 0, w << GRID_X_SHIFT, 1 << GRID_Y_SHIFT); 
		  XCopyPlane(stage.Dis, pm,  stage.Win, stage.bonw,
		      0, 0, w << GRID_X_SHIFT, 1 << GRID_Y_SHIFT,
		      i << GRID_X_SHIFT, j << GRID_Y_SHIFT, 1); 
		}
	      else
#endif
	      XPutImage(stage.Dis, stage.Win, stage.bonw, stage.Work, 
			i << GRID_X_SHIFT, j << GRID_Y_SHIFT, 
			i << GRID_X_SHIFT, j << GRID_Y_SHIFT, 
			w << GRID_X_SHIFT, 1 << GRID_Y_SHIFT); 
	      g += w;
	      i += w;
	      break;
	    default:
	      g++;
	      i++;
	  break;
	    }
	}
    }
  XSync(stage.Dis, False);
  return 0;
}

void drawinit()
{
  XGCValues gcv;
  char *p, buf1[256], buf2[256];
  struct passwd *lp;
  int i;

  gcv.clip_mask = None;
  XChangeGC(stage.Dis, stage.wonb, GCClipMask, &gcv);
  XChangeGC(stage.Dis, stage.bonw, GCClipMask, &gcv);
  XFillRectangle(stage.Dis, stage.Win, stage.bonw, 
		 0, 0, stage.screenx, stage.screeny);

  ImOverImGrid(stage.Work, stage.Clock, NULL,
	   stage.screenx - stage.Clock->width - 100, 
	   stage.screeny - stage.Clock->height, 
	   0, 0, stage.Clock->width, stage.Clock->height, stage.grid);
  ImOverImGrid(stage.Work, stage.Scale, NULL, 
	   stage.ticks->x, stage.ticks->y, 0, 0, 
	   stage.ticks->an->width, stage.ticks->an->height, stage.grid);
  bcopy(stage.Work->data, stage.Back->data, 
	stage.Work->bytes_per_line * stage.Work->height);
  DrawFrameGrid(stage.ticks, stage.grid);
  UpdateDisplayFromGrid();
  EraseFrameGrid(stage.ticks, stage.grid);
  UpdateWorkFromBack();

  XFillRectangle(stage.Dis, stage.Win, stage.wonb, 
		 LABEL_OFFSET_X - 3, LABEL_OFFSET_Y - 6, 157, 133);
  for (i = 0; labeltext[i] != NULL; i++)
    XDrawImageString(stage.Dis, stage.Win, stage.bonwl, LABEL_OFFSET_X,
                     LABEL_OFFSET_Y + (i + 1) * 16,
                     labeltext[i], strlen(labeltext[i]));
  if (options.speed < 60)
    sprintf(buf1, "15 * %d Sec.", options.speed);
  else
    sprintf(buf1, "%d Min.", options.speed / 4);
  XDrawImageString(stage.Dis, stage.Win, stage.bonwl, LABEL_OFFSET_X,
		   LABEL_OFFSET_Y + (i + 1) * 16, buf1, strlen(buf1));
  lp = getpwuid(getuid());
  gethostname(buf2, 256);
  if((p = index(lp->pw_gecos, ',')) != NULL)
    *p = 0;
		if (options.anon) LoginName="administration";
		else LoginName = strdup(*lp->pw_gecos ? lp->pw_gecos : lp->pw_name);
  sprintf(buf1, "Station %s parked by %s", buf2, LoginName);

  XDrawImageString(stage.Dis, stage.Win, stage.wonb, TEXT_XP, TEXT_YP, 
		   buf1, strlen(buf1));
  XDrawImageString(stage.Dis, stage.Win, stage.wonb, TEXT_XP, TEXT_YP+30, 
		   "Password:", 9);
  XFlush(stage.Dis);
}

int
play_animation()
{
  struct movement *m;
  int out, in, end, maxlems, n, i;
  struct timeval tstamp, now;
  struct timezone tz;
  int ex_size;
  int x = 0, y = 0;
  char buf[MAXPATHLEN];

  if (options.vacation)
    {
      /* 
       * this will give sad faces:
       */
      if (!stage.vac_top || !stage.vac_bot)
	{
	  fprintf(stderr, "cant play animation, no vac\n");
	  return 1;
	}
      sprintf(buf, "%s/%s/%s", plockdir, sounddir, SND_EXPLODE);
      if (!options.quiet)
	options.quiet = PlaySound(buf, GAIN);
      while (!DrawFrameGrid(stage.vac_bot, stage.grid))
	{
	  DrawFrameGrid(stage.vac_top, stage.grid);
	  UpdateDisplayFromGrid();
	  mu(0, MU_BLOCK);
	  EraseFrameGrid(stage.vac_top, stage.grid);
	  EraseFrameGrid(stage.vac_bot, stage.grid);
	  UpdateWorkFromBack();
	}
      WaitSoundPlayed();
      mu(10000, MU_BLOCK);
      /*
       * was that really all? Ohhhh...
       */
      return 0;
    }
  if (!stage.box_in || !stage.box_top || !stage.box_open)
    {
      fprintf(stderr, "cant play animation, no box\n");
      return 1;
    }

  sprintf(buf, "%s/%s/%s", plockdir, sounddir, SND_NEWLEVEL);
  if (!options.quiet)
    options.quiet = PlaySound(buf, GAIN);

  gettimeofday(&tstamp, &tz);
  while (!DrawFrameGrid(stage.box_in, stage.grid))
    {
      UpdateDisplayFromGrid();
      mu(0, MU_BLOCK);
    }

  gettimeofday(&now, &tz);
  i = (now.tv_sec - tstamp.tv_sec) * 1000 + 
      (now.tv_usec - tstamp.tv_usec) / 1000;
  maxlems  = 1 + LEMSPEED_BASE / i;

  if (options.speed < 60 || options.harmless || options.nolock)
    printf("Lemming based %smachine speed: %5.2f = 1 + %d / %d\n", 
	   stage.depth > 1 ? "color " : "",
	   1.0 + (double)LEMSPEED_BASE / (double)i, LEMSPEED_BASE, i);

  sprintf(buf, "%s/%s/%s", plockdir, sounddir, SND_BOXOPEN);
  if (!options.quiet)
    options.quiet = PlaySound(buf, GAIN);

  while (!DrawFrameGrid(stage.box_open, stage.grid))
    {
      UpdateDisplayFromGrid();
      EraseFrameGrid(stage.box_open, stage.grid);
      UpdateWorkFromBack();
      mu(0, MU_BLOCK);
    }

  /*
   * Now we burn the open box into the background
   */
  m = stage.box_open;
  ImOverIm(stage.Back, stage.Work, NULL, m->prev_x, m->prev_y,
	   m->prev_x, m->prev_y, m->an->width, m->an->height);
  m = stage.ticks;
  ImOverIm(stage.Back, stage.Work, NULL, m->prev_x, m->prev_y,
	   m->prev_x, m->prev_y, m->an->width, m->an->height);

  /* 
   * and now here come all lemmings
   */
  sprintf(buf, "%s/%s/%s", plockdir, sounddir, SND_LETSGO);
  if (!options.quiet)
    options.quiet = PlaySound(buf, GAIN);
  if (maxlems > stage.nlems)
    maxlems = stage.nlems;

  if (!maxlems)
    return 0;
  out = 0;
  in = 0;
  ex_size = 128;
  for (n = 0; in < maxlems; n++)
    {
      end = 0;
      for (i = 0; i < maxlems; i++)
    	end += DrawLem(i);
      if (end)
	{
	  x = stage.lem[0].fire->x - ex_size/2;
	  y = stage.screeny - ex_size;
	  sprintf(buf, "%s/%s/%s", plockdir, sounddir, SND_OHNO);
	  if (!options.quiet)
	    {
	      options.quiet = PlaySound(buf, GAIN);
	      WaitSoundPlayed();
	      sprintf(buf, "%s/%s/%s", plockdir, sounddir, SND_EXPLODE);
	      if (!options.quiet)
		options.quiet = PlaySound(buf, GAIN);
	    }
          for (i = 1; i < maxlems; i++)
    	    EraseLem(i);
          UpdateWorkFromBack();
	  ImFillRectangle(stage.Work, x, y, ex_size, ex_size,
			  stage.red, stage.grid);
	  DrawLem(0);
	  UpdateDisplayFromGrid();
	  in++;
	  play_explosion(x, y, ex_size);
	  WaitSoundPlayed();
	  return 0;
	}
      else
	{
	  UpdateDisplayFromGrid();
          for (i = 0; i < maxlems; i++)
    	    EraseLem(i);
          UpdateWorkFromBack();
	}
      if (((n % stage.release_rate) == 0) && out < maxlems)
	{
	  stage.lem[out++].status = LEM_START;
	}
      mu(50, MU_BLOCK);
    }
  return 0;
}

int
EraseLem(i)
int i;
{
  struct lem_move *l;

  l = stage.lem + i;
  i = 0;

  switch (l->prev_status)
    {
    case LEM_START:
      EraseFrameGrid(l->faller1, stage.grid);
      break;
    case LEM_WALK1:
      EraseFrameGrid(l->walker1, stage.grid);
      break;
    case LEM_FALL2:
      EraseFrameGrid(l->faller2, stage.grid);
      break;
    case LEM_WALK2:
      EraseFrameGrid(l->walker2, stage.grid);
      break;
    case LEM_BOMB:
      EraseFrameGrid(l->bomber, stage.grid);
      EraseFrameGrid(l->numbers, stage.grid);
      EraseFrameGrid(l->fire, stage.grid);
      break;
    default:
      break;
    }
  return 0;
}

int DrawLem(i)
int i;
{
  struct lem_move *l;

  l = stage.lem + i;
  i = 0;

  switch (l->status)
    {
    case LEM_DEAD:
      if (l->status != l->prev_status)
	{
	  l->prev_status = l->status;
	  return 1;
	}
      return 0;
    case LEM_NOTSTARTED:
      return 0;
    case LEM_START:
      i =  DrawFrameGrid(l->faller1, stage.grid);
      DrawFrameGrid(stage.box_top, stage.grid);
      break;
    case LEM_WALK1:
      i =  DrawFrameGrid(l->walker1, stage.grid);
      break;
    case LEM_FALL2:
      i =  DrawFrameGrid(l->faller2, stage.grid);
      break;
    case LEM_WALK2:
      i =  DrawFrameGrid(l->walker2, stage.grid);
      break;
    case LEM_BOMB:
      DrawFrameGrid(l->bomber, stage.grid);
      DrawFrameGrid(l->fire, stage.grid);
      DrawFrameGrid(l->numbers, stage.grid);
      if ((l->lifetime % 32) == 0)
	{
	  if (l->numbers->frame-- <= 0)
	    {
	      return 1;
	    }
	}
      if ((l->lifetime % 20) == 0)
	{
	  frame_adjust(l->fire, -1, 1);
	}
      break;
    default:
      return -1;
    }

  l->lifetime++;
  if (i == 1)
    {
      /* this animation part is finished */
      l->prev_status = l->status++;
      l->lifetime = 1;
    }
  else
    l->prev_status = l->status;
  return 0;
}

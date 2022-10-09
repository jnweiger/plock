/*
 * explode.c -- plock lemming explosion.
 * This code assumes, that there is a rectangle of interesting graphics
 * (like a lemming carrying a bomb on bloody red background) that should be
 * splashed across the screen.
 */
#include "rcs.h"
RCS_ID("$Id$ FAU")

#include "plock.h"

#define MAX_PARTS		256 /* max nr. of alloced particles. */
#define PARTICLE_MIN_SIZE 	4   /* smallest flying object */
#define FLIGHT			2   /* we decrease size on every nth crack */
#define GRAVITY 		2   /* in our world 9.81 m/sec^2 */
#define CRACK_FORCE		11  /* acceleration when creacked */
#define QUEUE_SIZE		800 /* number of visible parts */
#define HSIZE(e)	((e)->size)
#define VSIZE(e)	((e)->size >> 1)

#ifdef SOLARIS
#define assert(a) { if (!(a)) { printf("assert,  l.%d\n", __LINE__); abort();}}
#else
#define assert(a) { if (!(a)) { printf("%s (%d)\n", "a", __LINE__); abort();}}
#endif

struct particle
{
  int x, y; 	/* position of top left corner */
  int x_off, y_off;
  int size;
  int speed_x, speed_y;
  int color;
  int flag;
  struct particle *l, *r;
} *ex;

static int pcount;

struct smear
{
  int x, y, w, h;
} smearqueue[QUEUE_SIZE];

static int smearqueueptr;

static Pixmap pm;
static long col, ccol;

#define new_part(p) { if (pcount >= MAX_PARTS) return 1; p = ex + pcount++; }
#define new_col(c) { c = (col >>= 1) & 1; if (!col) col = ccol = ~ccol; }

extern int expl_init();
static int explode_particle();
static int enqueue_particle();
static int expl_crack();

/*
 * give a square size x size with top lefthand corner at x, y
 */
int
play_explosion(x, y, size)
int x, y, size;
{
  int i;

  expl_init(x, y, 5, -47, size);
  mu(0, 0);
  for (i = 0; i < 200; i++)
    {
      explode_particle(ex);
      XFlush(stage.Dis);
      mu(20, MU_BLOCK);
    }
  mu(0, 0); mu(2000, MU_BLOCK);
  XFillRectangle(stage.Dis, stage.Win, stage.bonw,
		 0, 0, stage.screenx, stage.screeny);
  XFlush(stage.Dis);
  Free(ex);
  return 0;
}

int
expl_init(x, y, xspeed, yspeed, size)
int x, y, xspeed, yspeed, size;
{
  register struct particle *p;
  register struct smear *s;
  GC gc;
  
  s = smearqueue;
  for (smearqueueptr = QUEUE_SIZE; smearqueueptr > 0; smearqueueptr--)
    {
      s->x = s->y = s->w = s->h = 0;
      s++;
    }
    
  if (y > stage.screeny - size)
    y = stage.screeny - size;
  pm = XCreatePixmap(stage.Dis, stage.Win, size, size, 
				DefaultDepth(stage.Dis, stage.Sc));
  gc = XCreateGC(stage.Dis, pm, 0, 0);
  XCopyArea(stage.Dis, stage.Win, pm, gc, x, y, size, size, 0, 0); 
  ImFillRectangle(stage.Work, x, y, size, size,
		  stage.black, stage.grid);
  UpdateDisplayFromGrid();

  pcount = 0;
  ex = (struct particle *)malloc(MAX_PARTS * sizeof(struct particle));
  new_part(p);

  p->x = x; p->y = y;
  p->size = size;
  p->x_off = p->y_off = 0;
  p->speed_x = xspeed; p->speed_y = yspeed;
  /* we must start with a square piece and a vertical crack */
  p->color = p->flag = 0;
  p->l = p->r = NULL;
  srand(time(0));
  ccol = col = rand();

  /*
    {
      XGCValues gcval;

      gcval.function = GXxor;
      XChangeGC(stage.Dis, stage.wonb, GCFunction, &gcval);
    }
  */

  return 0;
}

static int 
move_particle(p)
struct particle *p;
{
  p->x += p->speed_x;
  p->y += p->speed_y;
  if (p->x >= 0 && p->y >= 0 && 
      p->x + p->size < stage.screenx && 
      p->y + p->size < stage.screeny)
    {
#if 0
      XFillRectangle(stage.Dis, stage.Win, stage.wonb,
		     p->x, p->y, p->size, p->size);
#else
      XCopyArea(stage.Dis, pm, stage.Win, stage.wonb, p->x_off, p->y_off, 
		p->size, p->flag ? (p->size << 1) : p->size, p->x, p->y);
#endif
    }
  enqueue_particle(p);
  return 0;
}

static int
enqueue_particle(p)
struct particle *p;
{
  register struct smear *s;

  s = smearqueue + smearqueueptr++;
  if (smearqueueptr >= QUEUE_SIZE)
    {
      smearqueueptr = 0;
    }
  if (s->w && s->h)
    XFillRectangle(stage.Dis, stage.Win, stage.bonw, s->x, s->y, s->w, s->h);
  if (p)
    {
      s->w = p->size;
      s->h = p->flag ? (p->size << 1) : p->size;
      s->x = p->x;
      s->y = p->y;
    }
  else
    s->w = s->h = s->x = s->y = 0;
  return 0;
}

int sch = 0;

static int
explode_particle(p)
struct particle *p;
{
  register int r = 0, s = 1, t = 1;

  sch++;
  if (p->l)
    s = explode_particle(p->l);
  if (p->r)
    t = explode_particle(p->r);
  r = expl_crack(p);
  if (p->l && (p->l->y > (int)stage.screeny) && (p->l->l == NULL) && (p->l->r == NULL))
    {
      debug1("%04d zap!\n", p->l-ex);
      p->l = NULL;
    }
  if (p->r && (p->r->y > (int)stage.screeny) && (p->r->l == NULL) && (p->r->r == NULL))
    {
      debug1("%04d zap!\n", p->r-ex);
      p->r = NULL;
    }


  if (p->l)
    move_particle(p->l);
  if (p->r)
    move_particle(p->r);
  move_particle(p);
  sch--;
  return r && s && t;
}

/*
 * one particle is split up in two particles of half size and one particle
 * of minimum size.
 */
static int
expl_crack(p)
struct particle *p;
{
  register struct particle *l, *r;
  register int off;

  p->speed_y += GRAVITY - (p->x % 5) + 2;

  if (p->size <= PARTICLE_MIN_SIZE || pcount >= MAX_PARTS)
    {
      move_particle(p);
      new_col(p->color);
      return 1;
    }
  assert(p->r == 0);
  assert(p->l == 0);

  new_part(l);
  new_part(r);
  /*
   * the new_part macros may do return 1; if not successfull
   * if they did not, we link in the new structures and initialize them
   */
  p->l = l;
  p->r = r;
  if (p->flag)
    {
      /* this will be a horizontal crack 
       * where left is top and right is bottom */
      off = p->size;
      l->speed_y = p->speed_y - CRACK_FORCE;
      r->speed_y = p->speed_y + CRACK_FORCE;
      l->speed_x = r->speed_x = p->speed_x;
      l->y = p->y;        l->y_off = p->y_off;
      r->x = l->x = p->x; r->x_off = l->x_off = p->x_off;
      r->y = p->y + off;  r->y_off = p->y_off + off;
      r->size = l->size = p->size;
      p->y += off; p->y_off += off;
      off >>= 1;
      p->x += off; p->x_off += off;
    }
  else
    {
      /* this will be a vertical crack */
      off = p->size >> 1;
      l->speed_x = p->speed_x - CRACK_FORCE;
      r->speed_x = p->speed_x + CRACK_FORCE;
      l->speed_y = r->speed_y = p->speed_y;
      l->x = p->x;        l->x_off = p->x_off;
      r->x = p->x + off;  r->x_off = p->x_off + off;
      r->y = l->y = p->y; r->y_off = l->y_off = p->y_off;
      r->size = l->size = off;
      p->x += off; p->x_off += off;
      p->y += off; p->y_off += off;
    }
  l->l = l->r = r->l = r->r = NULL;
  new_col(p->color = l->color = r->color);
  l->flag = r->flag = !p->flag;
  p->size = PARTICLE_MIN_SIZE;

#if DEBUG
  printf("%d, %d: p=%04d r=%04d l=%04d x=%d y=%d flag=%d size=%d\n",
	 pcount, sch, p-ex, p->r-ex, p->l-ex, p->x, p->y, p->flag, p->size);
#endif
  return 0;
}

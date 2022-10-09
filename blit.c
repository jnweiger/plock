/*
 * blit.c -- plock image composition with an amiga-like blit routine
 */
#include "sccs.h"
SCCS_ID("@(#)blit.c	1.2 92/02/21 17:45:51 FAU");

void
im2im(sim, sbpl, sx, sy,
      mim, mbpl, mx, my,
      dim, dbpl, dx, dy,
      w, h)
char *sim, *mim, *dim;
int sbpl, mbpl, dbpl;
int sx, sy, dx, dy, mx, my;
int w, h;
{
  char *a, *m, *d;
  int drop = 0, ww, mshift, ashift;
  void blit();

  d = dim + dy * dbpl + (dx >> 5) * 4;
  ww = (dx + w - 1 >> 5) * 4 - (dx >> 5) * 4 + 4;
  ashift = (dx & 31) - (sx & 31);
  a = sim + sy * sbpl + (sx >> 5) * 4;

  if (mim == 0)
    {
      if (ashift < 0)
	{
	  ashift += 32;
	  drop = 1;
	}
      blit((unsigned long *)a,            sbpl - ww - drop * 4,
	   (unsigned long *)0,            0,
	   (unsigned long *)0,            0,
	   (unsigned long *)d,            dbpl - ww,
	   ww, h, 0xf0, ashift, 0, drop,
	   ((unsigned long)0xffffffff >> (dx & 31)),
	   ((unsigned long)0xffffffff << ((dx + w - 1 & 31) ^ 31)));
    }
  else
    {
      mshift = (dx & 31) - (mx & 31);
      m = mim + my * mbpl + (mx >> 5) * 4;
      if (ashift < 0)
	{
	  ashift += 32;
	  if (mshift < 0)
	    mshift += 32;
	  else
	    m -= 4;
	  drop = 1;
	}
      else
	{
	  if (mshift < 0)
	    {
	      mshift += 32;
	      a -= 4;
	      drop = 1;
	    }
	}
      blit((unsigned long *)a,              sbpl - ww - drop * 4,
	   (unsigned long *)m,              mbpl - ww - drop * 4,
	   (unsigned long *)(d - drop * 4), dbpl - ww - drop * 4,
	   (unsigned long *)d,              dbpl - ww,
	   ww, h, 0xe2, ashift, mshift, drop,
	   ((unsigned long)0xffffffff >> (dx & 31)),
	   ((unsigned long)0xffffffff << ((dx + w - 1 & 31) ^ 31)));
    }
}

void
blit(a, ma, b, mb, c, mc, d, md, w, h, mint, sa, sb, dropfl, fmd, lmd)
register unsigned long *a,*b,*c,*d;
unsigned long fmd, lmd;
int ma,mb,mc,md;
int sa,sb, dropfl, w, h, mint;
{
  register unsigned long xa, xb, xc, xd;
  unsigned long oa, ob;
  int ww, drop;

  xa = xb = xc = xd = 0;
  while(h--)
    {
      drop = dropfl;
      ww = w;
      oa = ob = 0;
      while (ww)
	{
	  if (a)
	    {
	      xa = *a++;
	      if (sa > 0)
		{
		  register unsigned long ooa;

		  ooa = xa;
		  xa = (xa >> sa) | (oa << (32 - sa));
		  oa = ooa;
		}
	    }
	  if (b)
	    {
	      xb = *b++;
	      if (sb > 0)
		{
		  register unsigned long oob;

		  oob = xb;
		  xb = (xb >> sb) | (ob << (32 - sb));
		  ob = oob;
		}
	    }
	  if (c)
	    xc = *c++;
	  switch(mint)
	    {
	    case 0xf0:
	      xd = xa;
	      break;
	    case 0xe2:
	      xd = xa & xb | xc & ~xb;
	      break;
	    default:
	      abort();
            }
	  if (drop)
	    drop = 0;
	  else
	    {
	      if (ww == w)
	        xd = xd & fmd | *d & ~fmd;
	      if ((ww -= 4) == 0)
	        xd = xd & lmd | *d & ~lmd;
	      *d++ = xd;
	    }
	}
      a = (unsigned long *)((unsigned char *)a + ma);
      b = (unsigned long *)((unsigned char *)b + mb);
      c = (unsigned long *)((unsigned char *)c + mc);
      d = (unsigned long *)((unsigned char *)d + md);
    }
}

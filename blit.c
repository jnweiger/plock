/*
 * blit.c -- plock image composition with an amiga-like blit routine
 */

/*
 * blit operates on XLONG data. Take care of alignment problems!
 */
#define XLONG unsigned long



#define SIZL (sizeof(XLONG))
#define BITL (sizeof(XLONG)*8)

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

  d = dim + dy * dbpl + (dx / BITL) * SIZL;
  ww = ((dx + w - 1) / BITL) * SIZL - (dx / BITL) * SIZL + SIZL;
  ashift = (dx & (BITL-1)) - (sx & (BITL-1));
  a = sim + sy * sbpl + (sx / BITL) * SIZL;

  if (mim == 0)
    {
      if (ashift < 0)
	{
	  ashift += BITL;
	  drop = 1;
	}
      blit((XLONG *)a,            sbpl - ww - drop * SIZL,
	   (XLONG *)0,            0,
	   (XLONG *)0,            0,
	   (XLONG *)d,            dbpl - ww,
	   ww, h, 0xf0, ashift, 0, drop,
	   ((XLONG)~0l >> (dx & (BITL-1))),
	   ((XLONG)~0l << (((dx + w - 1) & (BITL-1)) ^ (BITL-1))));
    }
  else
    {
      mshift = (dx & (BITL-1)) - (mx & (BITL-1));
      m = mim + my * mbpl + (mx / BITL) * SIZL;
      if (ashift < 0)
	{
	  ashift += BITL;
	  if (mshift < 0)
	    mshift += BITL;
	  else
	    m -= SIZL;
	  drop = 1;
	}
      else
	{
	  if (mshift < 0)
	    {
	      mshift += BITL;
	      a -= SIZL;
	      drop = 1;
	    }
	}
      blit((XLONG *)a,                 sbpl - ww - drop * SIZL,
	   (XLONG *)m,                 mbpl - ww - drop * SIZL,
	   (XLONG *)(d - drop * SIZL), dbpl - ww - drop * SIZL,
	   (XLONG *)d,                 dbpl - ww,
	   ww, h, 0xe2, ashift, mshift, drop,
	   ((XLONG)~0l >> (dx & (BITL-1))),
	   ((XLONG)~0l << (((dx + w - 1) & (BITL-1)) ^ (BITL-1))));
    }
}

void
blit(a, ma, b, mb, c, mc, d, md, w, h, mint, sa, sb, dropfl, fmd, lmd)
register XLONG *a,*b,*c,*d;
XLONG fmd, lmd;
int ma,mb,mc,md;
int sa,sb, dropfl, w, h, mint;
{
  register XLONG xa, xb, xc, xd;
  XLONG oa, ob;
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
		  register XLONG ooa;

		  ooa = xa;
		  xa = (xa >> sa) | (oa << (BITL - sa));
		  oa = ooa;
		}
	    }
	  if (b)
	    {
	      xb = *b++;
	      if (sb > 0)
		{
		  register XLONG oob;

		  oob = xb;
		  xb = (xb >> sb) | (ob << (BITL - sb));
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
	      xd = (xa & xb) | (xc & ~xb);
	      break;
	    default:
	      abort();
            }
	  if (drop)
	    drop = 0;
	  else
	    {
	      if (ww == w)
	        xd = (xd & fmd) | (*d & ~fmd);
	      if ((ww -= SIZL) == 0)
	        xd = (xd & lmd) | (*d & ~lmd);
	      *d++ = xd;
	    }
	}
      a = (XLONG *)((unsigned char *)a + ma);
      b = (XLONG *)((unsigned char *)b + mb);
      c = (XLONG *)((unsigned char *)c + mc);
      d = (XLONG *)((unsigned char *)d + md);
    }
}

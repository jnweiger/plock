/*
 * image.c -- plock image manipulation for lemmings
 */
#include "sccs.h"
SCCS_ID("@(#)image.c	1.2 92/02/21 17:45:55 FAU");

#define SLOW_COPY 0

  static unsigned int shifts[8] = 
  {
    (0x80 >> 0), (0x80 >> 1), (0x80 >> 2), (0x80 >> 3),
    (0x80 >> 4), (0x80 >> 5), (0x80 >> 6), (0x80 >> 7),
  };
#if SLOW_COPY
  static unsigned int nshifts[8] = 
  {
    ~(0x80 >> 0), ~(0x80 >> 1), ~(0x80 >> 2), ~(0x80 >> 3),
    ~(0x80 >> 4), ~(0x80 >> 5), ~(0x80 >> 6), ~(0x80 >> 7),
  };
#endif

#include "plock.h"


#define MICHA
void im2im();

/*
# include "copy_it.c"
*/

int
ImOverImGrid(Back, Front, Mas, x_b_off, y_b_off, 
	     x_f_off, y_f_off, width, height, grid)
XImage *Back, *Front, *Mas;
int x_b_off, y_b_off, x_f_off, y_f_off, width, height;
char *grid;
{
  int sx, sy, x, y, w, h;
  register int line, col;
  int f_off, b_off, m_off; /* offsets in pixel */
  register unsigned char *fp, *bp;
#if 0
  register unsigned char *mp = 0;
#endif
                         /* pointer to the front,back,mask data */
  
  sx = Back->width;
  sy = Back->height;
  x = x_b_off;
  y = y_b_off;
  w = width;
  h = height;

  /* 
   * consistency checks:
   */
  if (!Back || !Front)
    return 1;
  if ((Back->depth != Front->depth) || (Back->depth != 1 && Back->depth != 8))
    return 2;
  if (Mas && (Mas->depth != 1) && 
      (Mas->width < Front->width) &&
      (Mas->height < Front->height))
    return 3;
  /*
   * range checks:
   */
  if (x + w < 0 || x > sx || y + h < 0 || y > sy)
    return 4;
  f_off = b_off = m_off = 0;
  if (x + w > sx)
    w = sx - x;
  if (x < 0)
    {
      w += x; 
      x_f_off -= x;
      x = 0;
    }
  if (y + h > sy)
    h = sy - x;
  /*
   * range checks for the source images
   */
  if (w <= 0 || h <= 0)
    return 5;
  if (x_f_off + w > Front->width || y_f_off + h > Front->height ||
      x_f_off < 0 || y_f_off < 0)
    return 6;
  if (grid)
    SetGrid(grid, x, y, x + w, y + h, GRID_UPDATE);
  switch (Back->depth)
    {
#if 0
    case 8:
      f_off += Front->bytes_per_line * y_f_off;
      if (Mas)
	{
	  mp = (unsigned char *)Mas->data + Mas->bytes_per_line * y_f_off;
	}
      if (y < 0)
        {
	  /* increases f_off, m_off !! */
          h += y; f_off -= Front->bytes_per_line * y;
	  y_f_off -= y;
          if (Mas)
	    mp -= Mas->bytes_per_line * y;
          y = 0;
        }
      b_off = x + Back->bytes_per_line * y;
      fp = (unsigned char *)Front->data + f_off;
      bp = (unsigned char *)Back->data + b_off; 
      
      if(Mas)
	{
	  register unsigned long startmbit;
	  register unsigned long mbit;
	  register unsigned long *mp2, mpdata;

	  mp += (m_off >> 5) * 4;
          startmbit = (unsigned long)0x80000000 >> (m_off & 31);
	  for (line = h; line--; )
	    {
	      mbit = startmbit;
	      mp2 = (unsigned long *)mp;
	      mpdata = *mp2++;
	      for (col = w; col--; )
		{
		  if (mpdata & mbit)
		    *bp++ = *fp++;
		  else
		    bp++, fp++;
		  if ((mbit >>= 1) == 0)
		    {
		      mbit = (unsigned long)0x80000000;
		      mpdata = *mp2++;
		    }
		}
	      fp += Front->bytes_per_line - w;
	      bp += Back->bytes_per_line - w;
	      mp += Mas->bytes_per_line;
	    }
	}
      else
	{
	  for (line = h; line--; )
	    {
	      for (col = w; col--; )
		{
	          *bp++ = *fp++;
		}
	      fp += Front->bytes_per_line - w;
	      bp += Back->bytes_per_line - w;
	    }
	}
      break; 
#else
    case 8:
      if (y < 0)
        {
          h += y; 
	  y_f_off -= y;
          y = 0;
	}
      f_off = Front->bytes_per_line * y_f_off + x_f_off;
      b_off = x + Back->bytes_per_line * y;
      fp = (unsigned char *)Front->data + f_off;
      bp = (unsigned char *)Back->data + b_off; 
      
      if(Mas)
	{
	  m_off = (Mas->bytes_per_line << 3) * y_f_off + x_f_off;
	  for (line = 0; line < h; line++)
	    {
	      for (col = 0; col < w; col++)
		{
		  if (Mas->data[m_off >> 3] & shifts[m_off & 0x07])
		    *bp = *fp;
		  m_off++; fp++; bp++;
		}
	      fp += Front->bytes_per_line - w;
	      bp += Back->bytes_per_line -w;
	      m_off += (Mas->bytes_per_line << 3) - w;
	    }
	}
      else
	{
	  for (line = 0; line < h; line++)
	    {
	      for (col = 0; col < w; col++)
		*bp++ = *fp++;
	      fp += Front->bytes_per_line - w;
	      bp += Back->bytes_per_line - w;
	    }
	}
      break; 
#endif
    case 1:
#if SLOW_COPY
      f_off += (Front->bytes_per_line << 3) * y_f_off;
      if (Mas)
	m_off += (Mas->bytes_per_line << 3) * y_f_off;
      if (y < 0)
        {
          h += y; f_off -= (Front->bytes_per_line << 3) * y;
	  y_f_off -= y;
          if (Mas)
            m_off -= (Mas->bytes_per_line << 3) * y; 
          y = 0;
        }
      {
	int mbpl, fbpl, bbpl;

        b_off = x + (Back->bytes_per_line << 3) * y;

	if (((w & 7) == 0) && ((b_off & 7) == 0) &&
	    ((m_off & 7) == 0) && ((f_off & 7) == 0))
	  {
	    w >>= 3;
	    mbpl = Mas ? Mas->bytes_per_line - w : 0;
	    fbpl = Front->bytes_per_line - w;
	    bbpl = Back->bytes_per_line - w;

	    if (Mas)
	      mp = Mas->data + (m_off >> 3);
	    fp = Front->data + (f_off >> 3);
	    bp = Back->data + (b_off >> 3);

            if(Mas)
	      {
		for (line = 0; line < h; line++)
		  {
		    for (col = 0; col < w; col++)
		      {
			*bp = (*bp & ~*mp) | (*fp & *mp);
			mp++; bp++; fp++;
		      }
		    mp += mbpl; bp += bbpl; fp += fbpl;
		  }
	      }
	    else
	      {
		for (line = 0; line < h; line++)
		  {
		    for (col = 0; col < w; col++)
		      {
			*bp = *fp; bp++; fp++;
		      }
		    fp += fbpl; bp += bbpl;
		  }
	      }
	  }
	else
	  {
	    mbpl = Mas ? (Mas->bytes_per_line << 3) - w : 0;
	    fbpl = (Front->bytes_per_line << 3) - w;
	    bbpl = (Back->bytes_per_line << 3) - w;
	    mp = Mas ? Mas->data : 0;
	    fp = Front->data;
	    bp = Back->data ;

	    for (line = 0; line < h; line++)
	      {
		for (col = 0; col < w; col++)
		  {
		    if (!Mas || (mp[m_off >> 3] & shifts[ m_off & 7 ]))
		      {
			if (fp[f_off >> 3] & shifts[ f_off & 7 ])
			  bp[b_off >> 3] |= shifts[ b_off & 7 ];
			else
			  bp[b_off >> 3] &= nshifts[ b_off & 7 ];
		      }
		    if (Mas)
		      m_off++;
		    f_off++; b_off++;
		  }
		m_off += mbpl; f_off += fbpl; b_off += bbpl;
	      }
	  }
      }
#else
      if (y < 0)
        {
          h += y;
	  y_f_off -= y;
          y = 0;
        }
# ifdef MICHA
      im2im(Front->data, Front->bytes_per_line, x_f_off, y_f_off, 
	    Mas ? Mas->data : 0, Front->bytes_per_line, x_f_off, y_f_off,
	    Back->data, Back->bytes_per_line, x, y,
	    w, h);
# else
      insert_image((t_img *)Back->data, (unsigned long)y, (unsigned long)x, 
		   (unsigned long)Back->bytes_per_line, 
		   Mas ? (t_img *)Mas->data : (t_img *)0,
		   (t_img *)Front->data, (unsigned long)y_f_off, 
		   (unsigned long)x_f_off, (unsigned long)Front->bytes_per_line,
		   (unsigned long)h, (unsigned long)w);
# endif
#endif
      break;
    }
  return 0;
}


int
ImOverIm(back, front, mask, xb, yb, xf, yf, w, h)
XImage *back, *front, *mask;
int xb, yb, xf, yf, w, h;
{
  return ImOverImGrid(back, front, mask, xb, yb, xf, yf, w, h, NULL);
}

/*
 * note, this code is strange, it rounds off the x position 
 * in case of a single bit image.
 */
int 
ImFillRectangle(im, x, y, w, h, c, grid)
XImage *im;
int x, y, w, h, c;
char *grid;
{
  int i, j;
  char *p;

  if (grid)
    SetGrid(grid, x, y, x + w, y + h, GRID_UPDATE);
  switch (im->depth)
    {
    case 8:
      break;
    case 1:
      w = (w + 7) / 8;
      x >>= 3;
      if (c)
       c = 0xff;
      break;
    default:
      abort();
    }

  for (j = 0; j < h; j++)
    {
      p = im->data + im->bytes_per_line * (y + j) + x;
      for (i = 0; i < w; i++)
	*p++ = c;
    }
  return 0;  
}
#define sccs_id sccs_id_blit
#include "blit.c"


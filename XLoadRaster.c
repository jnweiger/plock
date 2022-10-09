/*
 * XLoadRaster.c -- plock loads rasterfiles
 *
 */
#include "rcs.h"
RCS_ID("$Id$ FAU")

#include <X11/Xlib.h>
#include <sys/errno.h>
#include <stdio.h>
#include "plock.h"
#include "ras.h"

extern int errno;

#if !defined(__GNUC__) && !defined(SOLARIS)
# ifdef sun
    extern int free();
    extern char *malloc();
# else
    extern void free();
    extern void *malloc();
# endif
#endif

#ifdef COMMENT
  valid bitmap_pads are: 8 16 24 32 ...
#endif

#define RAS_ESC 128

static void my_free(cm, d)
  colormap_t *cm;
  char *d;
{
  if(cm)
    {
      if(cm->map[0]) free(cm->map[0]);
      if(cm->map[1]) free(cm->map[1]);
      if(cm->map[2]) free(cm->map[2]);
      cm->map[0] = cm->map[1] = cm->map[2] = 0;
    }
  if(d)
    {
      free(d); *d = 0;
    }
}

#ifdef LOADDEBUG
#define GETC(c, fp) if((c=getc(fp)) == EOF) {errno=EINVAL; debug("GETC\n"); my_free(cmap, data); return NULL;} 
#else
#define GETC(c, fp) if((c=getc(fp)) == EOF) {errno=EINVAL; my_free(cmap, data); return NULL;} 
#endif

static unsigned int shifts[8] = 
{
  (0x80 >> 0), (0x80 >> 1), (0x80 >> 2), (0x80 >> 3),
  (0x80 >> 4), (0x80 >> 5), (0x80 >> 6), (0x80 >> 7),
};

int
Image1to8(imp, bitmap_pad)
XImage **imp;
int bitmap_pad;
{
  XImage *im, *new_im;
  int i, j, new_bpl;
  char *new_data, *p, *q;

  im = *imp;
  if (stage.depth == 1 || im->depth == 8)
    return 0;
  new_bpl = im->width << 3;
  if ((i = new_bpl % (bitmap_pad/8)) != 0) 
    new_bpl += (bitmap_pad/8) - i;
  if ((new_data = (char *)malloc(im->height * new_bpl)) == NULL)
    {
      errno = ENOMEM; 
      return 0;
    }

  for (j = 0; j < im->height; j++)
    {
      p = new_data + j * new_bpl;
      q = im->data + j * im->bytes_per_line;
      for (i = 0; i < im->width; i++)
	*p++ = (q[i>>3] & shifts[i&7]) ? stage.black : stage.white;
    }

  new_im = XCreateImage(stage.Dis, stage.vis, stage.depth,
		        ZPixmap, 0, new_data,
		        im->width, im->height,
		        bitmap_pad, new_bpl);
  if (!new_im)
    {
      Free(new_data);
      return -1;
    }
  XDestroyImage(im);
  *imp = new_im;
  return 0;
}

XImage *
XLoadRasterfile(dis, vis, fp, cmap, bitmap_pad)
  Display *dis;
  Visual *vis;
  FILE *fp;
  colormap_t *cmap;
  int bitmap_pad;
{
  struct rasterfile rf;
  XImage *im;
  int i, j, k, l, c = 0, bpl, bpl2, count;
  char *data = 0, *dp;

  if(!fread((char *)&rf, sizeof(rf), 1, fp) || rf.ras_magic != RAS_MAGIC)
    {
      debug(" != RAS_MAGIC\n");
      errno = EINVAL; return NULL;
    }
  if(! (rf.ras_type == RT_STANDARD || rf.ras_type == RT_BYTE_ENCODED) ||
     ! (rf.ras_depth == 1 || rf.ras_depth == 8) ||
     ! (rf.ras_maptype == RMT_NONE || rf.ras_maptype == RMT_EQUAL_RGB) ||
     ! (rf.ras_width || rf.ras_height))
    {
      debug(" no rt. ... \n");
      errno = EINVAL; return NULL;
    }

  bpl = (rf.ras_depth == 1) ? (rf.ras_width + 7) / 8 : rf.ras_width;
  i = bpl % (bitmap_pad/8); 
  if (i)
    bpl += (bitmap_pad/8) - i;

  if (!(data = malloc(rf.ras_height * bpl)))
    {
      errno = ENOMEM; return NULL;
    }

  cmap->type = rf.ras_maptype;
  cmap->length = rf.ras_maplength;
  if(rf.ras_maptype == RMT_EQUAL_RGB)
    {
      cmap->map[0] = (unsigned char *)malloc(cmap->length / 3);
      cmap->map[1] = (unsigned char *)malloc(cmap->length / 3);
      cmap->map[2] = (unsigned char *)malloc(cmap->length / 3);
      if(!cmap->map[0] || !cmap->map[1] || !cmap->map[2])
	{
	  errno = ENOMEM; 
	  my_free(cmap, data);
	  return NULL;
	}
      if(!fread((char *)cmap->map[0], cmap->length / 3, 1, fp) ||
         !fread((char *)cmap->map[1], cmap->length / 3, 1, fp) ||
         !fread((char *)cmap->map[2], cmap->length / 3, 1, fp))
	{
	  debug(" bad cmap \n");
	  errno = EINVAL;
	  my_free(cmap, data);
	  return NULL;
	}
    }

  if(rf.ras_type == RT_STANDARD)
    {
      bpl2 = ((rf.ras_depth == 1 ? (rf.ras_width + 7) / 8 : rf.ras_width) + 1) & ~1;
      dp = data;
      for(i = 0; i < rf.ras_height; i++)
	{
	  if(!fread(dp, bpl2, 1, fp))
	    {
	      debug(" bad data\n");
	      errno = EINVAL;
	      my_free(cmap, data);
	      return NULL;
	    }
	  dp += bpl;
	}
    }
  else
    {
#ifdef LOADDEBUG
      debug("BYTE_ENCODED\n");
#endif
      dp = data;
      j = bpl2 = ((rf.ras_depth == 1 ? (rf.ras_width + 7) / 8 : rf.ras_width) + 1) & ~1;
      i = rf.ras_height;
      l = bpl2 % (bitmap_pad / 8); 
      if (l)
	l = (bitmap_pad/8) - l;

      for (count = 0; ; count--)
	{
	  if (count == 0)
	    {
	      GETC(c, fp);
	      count = 1;
	      if (c == RAS_ESC)
		{
		  GETC(k, fp);
#ifdef LOADDEBUG
		  debug1("RAS_ESC, k=%d\n", k);
#endif
		  if (k != 0)
		    {
		      count += k;
		      GETC(c, fp);
		    }
		}
	    }
	  *dp++ = c;
	  if (--j == 0)
	    {
	      j = bpl2;
	      dp += l;
	      if(--i == 0)
		break;
	    }
	}
    }
  im = XCreateImage(dis, vis, rf.ras_depth,
		    rf.ras_depth == 1 ? XYPixmap : ZPixmap,
		    0, (char *)data,
		    (unsigned)rf.ras_width, (unsigned)rf.ras_height, 
		    bitmap_pad, bpl);

  if(!im) 
    {
      debug("XCreateImage\n");
      errno = EINVAL;
      my_free(cmap, data);
      return NULL;
    }
  return im;
}

/*
 * snd.c -- plock plays sound nonblocking on a sparc
 */
#include "rcs.h"
RCS_ID("$Id$ FAU")

#include "plock.h"

#if defined (sparc)
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/file.h>
#ifdef SOLARIS
# include <sys/audioio.h>
#else
# include <sun/audioio.h>
#endif

# define	AUDIO_IODEV	"/dev/audio"
# define	DEFAULT_GAIN		40

static FILE *afp = NULL;
static audio_info_t ai;

int
PlaySound(name, gain)
char *name;
int gain;
{
  audio_info_t new_ai;
  int c, afd;
  FILE *fp;

  if(!stage.islocal) return;
  if (!gain)
    gain = DEFAULT_GAIN;
  if (!afp)
    {
      if ((afd = open(AUDIO_IODEV, O_NDELAY | O_WRONLY)) < 0)
	{
	  perror(AUDIO_IODEV);
	  return -1;
	}
      if ((afp = fdopen(afd, "w")) == NULL) 
	{
	  perror("Oh No not fdopened");
	  close(afd);
	  return -1;
	}
      if (ioctl(fileno(afp), AUDIO_GETINFO, (caddr_t)&ai))
	{
	  perror("audio getinfo error");
	  fclose(afp);
	  afp = NULL;
	  perror("AUDIO_GETINFO");
	  return -1;
	}
    }

  if ((fp = fopen(name, "r")) == NULL)
    {
      perror(name);
      WaitSoundPlayed();
      return -1;
    }

  new_ai = ai;
  new_ai.play.port = AUDIO_SPEAKER;
  new_ai.play.gain = gain;
  new_ai.play.pause = 0;

  if (ioctl(fileno(afp), AUDIO_SETINFO, (caddr_t)&new_ai))
    perror("audio setinfo");

  while ((c = getc(fp)) != EOF)
    putc(c, afp);
  fflush(afp);
  write(fileno(afp), 0, 0); /* place an EOF in the stream :-) */
  fclose(fp);
  return 0;
}

int
WaitSoundPlayed()
{
  if(!stage.islocal) return;
  if (afp)
    {
      if (ioctl(fileno(afp), AUDIO_DRAIN, (caddr_t)NULL))
	perror("audio drain error");
      if (ioctl(fileno(afp), AUDIO_SETINFO, (caddr_t)&ai))
	perror("audio setinfo error");
      fclose(afp);
      afp = NULL;
    }
  return 0;
}  
#endif /* sparc */




#if defined (sgi)

#include <audio.h>

static char utoc[256] =
{
0x81, 0x85, 0x89, 0x8d, 0x91, 0x95, 0x99, 0x9d,
0xa1, 0xa5, 0xa9, 0xae, 0xb2, 0xb6, 0xba, 0xbe,
0xc1, 0xc3, 0xc5, 0xc7, 0xc9, 0xcb, 0xcd, 0xcf,
0xd1, 0xd3, 0xd5, 0xd7, 0xd9, 0xdb, 0xdd, 0xdf,
0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0,
0xf1, 0xf1, 0xf2, 0xf2, 0xf3, 0xf3, 0xf4, 0xf4,
0xf5, 0xf5, 0xf6, 0xf6, 0xf7, 0xf7, 0xf8, 0xf8,
0xf9, 0xf9, 0xf9, 0xf9, 0xfa, 0xfa, 0xfa, 0xfa,
0xfb, 0xfb, 0xfb, 0xfb, 0xfc, 0xfc, 0xfc, 0xfc,
0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd, 0xfd,
0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe, 0xfe,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x7f, 0x7b, 0x77, 0x73, 0x6f, 0x6b, 0x67, 0x63,
0x5f, 0x5b, 0x57, 0x52, 0x4e, 0x4a, 0x46, 0x42,
0x3f, 0x3d, 0x3b, 0x39, 0x37, 0x35, 0x33, 0x31,
0x2f, 0x2d, 0x2b, 0x29, 0x27, 0x25, 0x23, 0x21,
0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18,
0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c, 0x0c,
0x0b, 0x0b, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08,
0x07, 0x07, 0x07, 0x07, 0x06, 0x06, 0x06, 0x06,
0x05, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04,
0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static int audio_initialized = 0, no_audio = 0;
static ALconfig audio_port_config;
static ALport audio_port;
static char audio_name[] = "Plock-Sounds";
#define	DEFAULT_GAIN		40
#define AUDIO_FILE_MAGIC        0x2e736e64


int
PlaySound(name, gain)
char *name;
int gain;
{
  char buf[1024];
  int params[6];
  int nr, i;
  FILE *fp;

  if(!stage.islocal) return;
  if(no_audio)
    return;
  gain /= 4;
  if(!audio_initialized)
    {
      int fd;
      if ((fd = open("/dev/hdsp/hdsp0master", O_RDWR)) < 0)
        {
	  perror("No sound on this sgi");
	  no_audio = 1;
	  return 0;
	}
      close(fd);
      audio_port_config = ALnewconfig();
      ALsetwidth(audio_port_config, AL_SAMPLE_8);
      ALsetchannels(audio_port_config, 1);
      ALsetqueuesize(audio_port_config, 8192);

      audio_port = ALopenport(audio_name, "w", audio_port_config);

      audio_initialized = 1;
    }
  if ((fp = fopen(name, "r")) == NULL)
    {
      perror(name);
      return -1;
    }

  if(!gain)
    gain = 40;
  params[0] = AL_OUTPUT_RATE;
  params[1] = AL_RATE_8000;
  params[2] = AL_LEFT_SPEAKER_GAIN;
  params[3] = (int)(2.56 * (double)gain);
  params[4] = AL_RIGHT_SPEAKER_GAIN;
  params[5] = (int)(2.56 * (double)gain);
  ALsetparams(AL_DEFAULT_DEVICE, params, 6);

  nr = fread(params, 4, 2, fp);
  if(params[0] != AUDIO_FILE_MAGIC)
    {
      fprintf(stderr, "Not an audio file...\n");
      fclose(fp);
    }
  fread(buf, 1, params[1]-4, fp);
  do
    {
      nr = fread(buf, 1, 1024, fp);
      for (i = 0; i < nr; i++)
      	buf[i] = utoc[(unsigned char)buf[i]];

      if(nr >= 0)
	ALwritesamps(audio_port, buf, nr);
    }
  while(nr == 1024);
  
  fclose(fp);
  return 0;
}

int
WaitSoundPlayed()
{
  if(!audio_initialized)
    return 0;
  while(ALgetfilled(audio_port) > 0)
    sginap(1);
  return 0;
}
#endif /* sgi */


#if !defined(sparc) && !defined(sgi)
int
WaitSoundPlayed()
{
  return 0;
}

int
PlaySound(name, gain)
char *name;
int gain;
{
  return 0;
}
#endif /* sgi, sparc */





void
set_bell_vol(dpy, percent)
Display *dpy;
int percent;
{
  XKeyboardControl values;

  values.bell_percent = percent;
  XChangeKeyboardControl(dpy, KBBellPercent, &values);
  return;
}

int
get_bell_vol(dpy)
Display *dpy;
{
  XKeyboardState state;
  
  XGetKeyboardControl(dpy, &state);
  return state.bell_percent;
}

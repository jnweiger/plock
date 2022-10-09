/*
 * snd.c -- plock plays sound nonblocking on a sparc
 */
#include "sccs.h"
SCCS_ID("@(#)snd.c	1.2 92/02/21 17:46:01 FAU");

#ifdef sparc
# include "plock.h"
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/file.h>
# include <sun/audioio.h>

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

#else

int
WaitSoundPlayed()
{
  return 0;
}

int
PlaySound(name)
char *name;
{
  return 0;
}

#endif

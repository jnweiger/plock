/*
 * plock.c
 *
 * This is version 0.1 -- beta release, use with care
 *
 * This program is distributed in the hope that it will be usefull, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include "sccs.h"
SCCS_ID("@(#)plock.c	1.20 92/02/21 17:45:57 FAU");

#include "plock.h"
#include "config.h"

#define LCK_KBD (1<<0)
#define LCK_PNT (1<<1)
#define LCK_HST (1<<2)
#define LCK_SCR (1<<3)

#define KILL_SOME 1
#define KILL_ALL  2
#define KILL_GENOCIDE  3

#ifndef SEC_PER_TICK
# define SEC_PER_TICK 60
#endif

struct Option options = 
{
  SEC_PER_TICK, 0, 0, 0, 0, 0, 0,
};
  
Colormap xmap, dmap;
char My_Dpy[256], *LoginName = NULL;
Window rootwin;
Cursor crs;
static int status = 0, badlog;
static int hostnr, ssto, ssiv, ssbl, ssex, pipe_fd[2];
XHostAddress *hosts;
Bool hoststate;
unsigned char empty[2] = { 0, 0 };

extern int errno;
extern char *sys_errlist[];
extern char *index(), *fgets();
void unlock();
void lock();
static void Init __P((void));

FILE *fp;

void waiter()
{
 int s;

 wait(&s);
}

void piper()
{
}

int dummy()
{
  debug("dummy\n");
  return 0;
}

static int shooter(sig, what)
  int what, sig;
{
  int ret, i, 
      yourpid, mypid, pid,
      myuid, uid;
  char buf[1024];

  mypid = getpid();
  myuid = getuid();

  debug2("shooter(%d, %d)\n", sig, what);
  if (what == KILL_GENOCIDE)
    {
      time(&i);
      sprintf(buf, "plock GENOCIDE (%d) %s\n", sig, ctime(&i));
      syslog(LOG_NOTICE, buf);
      for (i = 0; i < NSIG; i++)
	signal(i, SIG_IGN);
      yourpid = getppid();
      for (i = 3; i <= MAXPID; i++)
#ifdef DEBUG
	if (i != mypid && i != yourpid)
#else
        if (i != mypid)
#endif
          kill(i, SIGHUP);
      time(&i);
      sleep(5);
      return 0;
    }
      
  signal(SIGPIPE, piper);

  if(pipe(pipe_fd))
    {
      fprintf(stderr, "Sorry - can't open pipe.\n");
      free_all();
    }
  switch(yourpid = fork())
    {
      case -1:
        perror("Sorry - can't fork.");
	sprintf(buf, "plock PANIC - can't fork: %s\n", sys_errlist[errno]);
	syslog(LOG_NOTICE, buf);
	shooter(SIGHUP, KILL_GENOCIDE);
	shooter(SIGKILL, KILL_GENOCIDE);
        free_all();
      case 0:
        close(pipe_fd[0]);
        close(1); dup(pipe_fd[1]);
        execl(PSPROG, "ps", PSARGS, (char *)0);
        fprintf(stderr, "Sorry - can't exec ps\n");
        exit(0);
      default:
        signal(SIGCLD, waiter);
        fp = fdopen(pipe_fd[0], "r"); 
        close(pipe_fd[1]);
        fgets(buf, 1024, fp);
        ret = 0;
        while(fgets(buf, 1024, fp) != NULL)
          {
            pid = atoi(buf+PS_PID_OFFSET);
            uid = atoi(buf+PS_UID_OFFSET);
	    if(uid != myuid)
	      continue;
	    if(what == KILL_ALL)
	      {
		if(pid != mypid && pid != yourpid)
		  {
		    kill(pid, sig);
		    ret++; 
		  }
	      }
	    else
	     {
		if(!strncmp(buf+PS_TTY_OFFSET, "co", 2)
#ifdef X_NOT_ON_CONSOLE
		   || !strncmp(buf+PS_AV0_OFFSET, "X", 1))
#else
		    )
#endif
		  {
		    if(pid != mypid && pid != yourpid)
		      {
			kill(pid, sig);
			ret++; 
		      }
		  }
	     }
          }
	fclose(fp);
    }
  return ret;
}

void plock_terminate()
{
  int n, i, tr, fd;
  char logbuf[256];

  debug1("plock_terminate %d\n", status);
  if(stage.depth > 1 && options.color && !options.nolock)
    {
      XInstallColormap(stage.Dis, dmap);
      XFlush(stage.Dis);
    }
#ifndef DEBUG
  for(i = 0; i < NSIG; i++)
    if (i != SIGCLD)
      signal(i, SIG_DFL);
#endif

#ifdef sun

#ifdef COMMENT
  Do a 'kbd_mode -a' after shooting...
#endif

  fd = open("/dev/kbd", O_WRONLY, 0);
#endif

  shooter(SIGHUP, KILL_SOME);
  sprintf(logbuf, "parklock: %s is being logged off NOW!\n", LoginName);
  syslog(LOG_NOTICE, logbuf);
  sleep(5);
  n = shooter(SIGTERM, KILL_SOME);
  if(n > 0) 
    {
      sleep(15);
      if ((i = shooter(SIGKILL, KILL_SOME)) != 0)
	{
	  sprintf(logbuf,
		  "parklock: WARNING: %d ignored SIGHUP. SIGKILL sent!\n", i);
	  syslog(LOG_NOTICE, logbuf);
	  sleep(10);
	}
    }

 /*
  * If X is still alive, then we go Berserk.
  */
  if(status & LCK_HST)
    {
      status &= ~LCK_HST;
      if (hosts)
        {
	  XAddHosts(stage.Dis, hosts, hostnr);
	  XFree((caddr_t)hosts);
	}
      if(hoststate == False)
        XDisableAccessControl(stage.Dis);
    }
  hosts = XListHosts(stage.Dis, &hostnr, &hoststate);
  debug1("Xopen %04x\n", status);
  if(XOpenDisplay(My_Dpy) != NULL)
    {
      sprintf(logbuf, "parklock: Lies, damned lies and %s (Sorry, %s)\n",
	      PSPROG, LoginName);
      syslog(LOG_NOTICE, logbuf);
      shooter(SIGHUP, KILL_GENOCIDE);
      shooter(SIGTERM, KILL_GENOCIDE);
      shooter(SIGKILL, KILL_GENOCIDE);
    }
  
#ifdef sun
  tr = TR_ASCII;
  if (fd != -1)
    {
      if (ioctl(fd, KIOCTRANS, (caddr_t) &tr))
        {
          perror("ioctl (KIOCTRANS)");
        }
      close(fd);
    }
#endif
  exit(-1);
}


void analyse(ptr)
  char *ptr;
{
  int i, j;
  char localbuf[256];
  struct passwd *lp;

  lp = getpwuid(getuid());
  strcpy(localbuf, lp->pw_passwd);

  if (options.nolock)
    {
      j = strcmp("nolock", ptr);
    }
  else
    {
      if (j = strcmp(crypt(ptr, localbuf), localbuf))
       {
         lp = getpwuid(0);
         strcpy(localbuf, lp->pw_passwd);
         j = strcmp(crypt(ptr, localbuf), localbuf);
       }
    }
  for (i = strlen(ptr); i >= 0; i--)
    {
      ptr[i] = 0;
    }
  if (j)
    {
      XDrawImageString(stage.Dis, stage.Win, stage.wonb, TEXT_XP, TEXT_YP+60,
                       "Login failed, try again.", 24);
      XSync(stage.Dis, False);
      badlog = 1;
    }
  else
    {
      free_all();
    }
}

int handler(dis, ev)
  Display *dis;
  XErrorEvent *ev;
{
  fprintf(stderr, "? XError : code: %d, major %d, minor %d\n",
          ev->error_code, ev->request_code, ev->minor_code);
  unlock();
  return 0;
}

void free_all()
{
  static int aaarrgh = 0;
  debug("free_all\n");
  unlock();
  if (LoginName)
    free(LoginName);

  if (aaarrgh++)
    abort();
  XFreeCursor(stage.Dis, crs);

  XFreeGC(stage.Dis, stage.wonb);
  XFreeGC(stage.Dis, stage.bonw);
  XFreeGC(stage.Dis, stage.bonwl);
  if (stage.Clock)
    XDestroyImage(stage.Clock);
  if (stage.Scale)
    XDestroyImage(stage.Scale);
  if (stage.Work)
    XDestroyImage(stage.Work);
  if (stage.Back)
    XDestroyImage(stage.Back);
  if (stage.ticks)
    DestroyMovement(stage.ticks);
  if (stage.box_in)
    DestroyMovement(stage.box_in);
  if (stage.box_open)
    DestroyMovement(stage.box_open);
  if (stage.vac_top)
    DestroyMovement(stage.vac_top);
  if (stage.vac_bot)
    DestroyMovement(stage.vac_bot);
  /*
   destroy LEMMING 
   */
  XCloseDisplay(stage.Dis);
  exit(0);
}

void lock()
{
  if (options.nolock)
    return;
  if(XGrabKeyboard(stage.Dis, stage.Win, False, GrabModeAsync, GrabModeAsync,
                   CurrentTime) != GrabSuccess)
    {
      fprintf(stderr, "Can't grab the keyboard!\n");
      free_all();
    }
  status |= LCK_KBD;

#if 0
  {
  Window w, d1, d2;
  int rx, ry, dx, dy;
  unsigned int l;

/* Capture the pointer - XGrabPointer Returns GrabNotViewable . hmmm*/
  XQueryPointer(stage.Dis, rootwin, &d1, &d2, &rx, &ry, &dx, &dy, &l);
  w = XCreateWindow(stage.Dis, rootwin, rx, ry, 1, 1, 0,
		      CopyFromParent, CopyFromParent, CopyFromParent, 0, 0);
  XMapWindow(stage.Dis, w);
  XFlush(stage.Dis);

  printf("%d, %d, %d (%d)\n", GrabNotViewable, AlreadyGrabbed, GrabFrozen, l);
  }
#endif
  if(XGrabPointer(stage.Dis, stage.Win, False, 0, GrabModeAsync, 
	    GrabModeAsync, None, crs, CurrentTime) != GrabSuccess)
    {
      fprintf(stderr, "Can't grab the pointer!\n");
      free_all();
    }
  status |= LCK_PNT;

  hosts = XListHosts(stage.Dis, &hostnr, &hoststate);
  if (!hoststate)
    {
      debug("he had no access control\n");
      XEnableAccessControl(stage.Dis);
    }
  XRemoveHosts(stage.Dis, hosts, hostnr);
  status |= LCK_HST;

  XGetScreenSaver(stage.Dis, &ssto, &ssiv, &ssbl, &ssex);
  XSetScreenSaver(stage.Dis, 0, 0, 0, 0);
  status |= LCK_SCR;
}

void
unlock()
{
  debug("unlock\n");
  if (options.nolock)
    return;
  if(status & LCK_KBD)
    {
      status &= ~LCK_KBD;
      XUngrabKeyboard(stage.Dis, CurrentTime);
    }
  if(status & LCK_PNT)
    {
      status &= ~LCK_PNT;
      XUngrabPointer(stage.Dis, CurrentTime);
    }
  if(status & LCK_HST)
    {
      status &= ~LCK_HST;
      if (hosts)
	{
	  XAddHosts(stage.Dis, hosts, hostnr);
	  XFree((caddr_t)hosts);
	}
      if(hoststate == False)
        XDisableAccessControl(stage.Dis);
    }
  if(status & LCK_SCR)
    {
      status &= ~LCK_SCR;
      XSetScreenSaver(stage.Dis, ssto, ssiv, ssbl, ssex);
    }
}

#define MBUFLEN 1024
static void eat_events()
{
  static char buf[MBUFLEN], *ptr = buf;
  char text[2];
  XEvent  event;
  KeySym  key;
  int i;

  while (XPending(stage.Dis))
    {
      XNextEvent(stage.Dis, &event);
      switch(event.type)
	{
	  case KeyPress:
	    i = XLookupString ((XKeyEvent *)&event, text, 10, &key, 0);
	    if(badlog)
	      {
		char bb[64];
		int i;

		for(i = 63; i >= 0; i--)
		  bb[i] = ' ';
		XDrawImageString(stage.Dis, stage.Win,
				stage.wonb, TEXT_XP, TEXT_YP+60, bb, 48);
		XSync(stage.Dis, False);
		badlog = 0;
	      }
	    if(key == XK_Return || key == XK_KP_Enter || key == XK_Linefeed)
	      {
		*ptr = 0;
		ptr = buf;
		analyse(buf);
	      }
	    else if(key == XK_BackSpace || key == XK_Delete)
	      {
		if(ptr > buf)
		  {
		    ptr--;
		  }
	      }
	    else 
	      if(key >= XK_space && key <= XK_asciitilde)
	        {
		  if(ptr < buf + MBUFLEN - 1);
		    {
		      *ptr++ = text[0];
		    }
		  *ptr = '\0';
	        }
	    break;
	  case  Expose:
	  case  EnterNotify:
            if(stage.depth > 1 && options.color && !options.nolock)
	      {
		XInstallColormap(stage.Dis, xmap);
		XSync(stage.Dis, False);
	      }
	    break;
	  default:
	    break;
	}
    }
}


void main(argc, argv)
  int argc;
  char **argv;
{
  XSetWindowAttributes xswa;
  XGCValues gcval;
  XVisualInfo vinfo;
  int end, i;
  XWMHints wmh;
  unsigned width, height;
  Pixmap pm;
  XFontStruct *fnt, *fntl;
  struct stat st_buf;
  XColor curcol;
  char logbuf[256];
  unsigned long pmask, pixels[256];

  argv++;
  while (argc-- > 1)
    {
      if (strcmp(*argv, "-harmless") == 0)
        {
          options.harmless = 1;
	  /*
	   * harmless implies demo.
	   * Thus we frustrate cheating like "plock -harmless; plock"
	   */
	  options.speed = 2;
        }
      else 
        {
          if (strcmp(*argv, "-demo") == 0)
            {
              options.speed = 2;
            }
          else
            {
              if (strcmp(*argv, "-noanim") == 0)
                {
                  options.noanim = 1;
                }
	      else
	        {
		  if (strcmp(*argv, "-nolock") == 0)
		    {
		      options.harmless = 1;
		      options.nolock = 1;
		    }
		  else
		    {
		      if (strcmp(*argv, "-color") == 0)
			{
			  options.color = 1;
			}
		      else
			{
			  if (strcmp(*argv, "-quiet") == 0)
			    {
			      options.quiet = 1;
			    }
			  else
			    {
			      fprintf(stderr, 
				  "Usage: plock [-demo] [-noanim] [-color]\n");
			      exit(-1);
			    }
			}
		    }
	        }
            }
        }
      argv++;
    }

  /* 
   * baeh :-)
   */
  i = ((time(0) & 0xff) * 17) % 100;
  debug2("%d %d\n", i, time(0));
  if (i >= ((options.speed < 60) ? 50 : 95))
    options.vacation = 1;

#ifdef COMMENT
  Allow locks only from the localhost
#endif

  if(!options.nolock)
    {
      gethostname(My_Dpy, 256);
      strcat(My_Dpy, ":0.0");
    }
  else
    {
      My_Dpy[0] = 0;
    }

  if((stage.Dis = XOpenDisplay(My_Dpy)) == NULL)
    {
      fprintf(stderr, "Can't connect server.\n");
      exit(-1);
    }

  if(!options.nolock)
    {
#ifdef sun
      if(stat("/dev/console", &st_buf))
	{
	  fprintf(stderr, "Can't stat /dev/console...\n");
	  exit(-1);
	}
      if(st_buf.st_uid != getuid())
	{
	  fprintf(stderr, "Lock is only possible from localhost.\n");
	  exit(-1);
	}
#endif
    }
  
#ifdef COMMENT
  We have to check, if ps is ok on this machine
#endif

#ifndef DEBUG
  if (!options.nolock)
    for(i = 0; i < NSIG; i++)
      signal(i, free_all);
#endif
  XSetErrorHandler(handler);

#ifdef COMMENT
  XSynchronize(stage.Dis, True);
  Fontloading...
#endif

  if((fnt = XLoadQueryFont(stage.Dis, FONT)) == NULL)
    {
      fprintf(stderr, "Can't load %s\n", FONT);
      if((fnt = XLoadQueryFont(stage.Dis, "fixed")) == NULL)
        {
          fprintf(stderr, "Can't load the fixed font... Bye.\n");
          exit(-1);
        }
    }
  if((fntl = XLoadQueryFont(stage.Dis, FONTL)) == NULL)
    {
      fprintf(stderr, "Can't load %s\n", FONTL);
      if((fntl = XLoadQueryFont(stage.Dis, "fixed")) == NULL)
        {
          fprintf(stderr, "Can't load the fixed font... Bye.\n");
          exit(-1);
        }
    }

  stage.Sc = DefaultScreen(stage.Dis);
  stage.depth = DefaultDepth(stage.Dis, stage.Sc);
  rootwin = RootWindow(stage.Dis, stage.Sc);
  width = DisplayWidth(stage.Dis, stage.Sc);
  height = DisplayHeight(stage.Dis, stage.Sc);
  stage.white = WhitePixel(stage.Dis, stage.Sc);
  stage.black = BlackPixel(stage.Dis, stage.Sc);

  bzero((char *)&curcol, sizeof(XColor));
  curcol.pixel = stage.black;
  /* "empty" cursor */
  pm = XCreatePixmapFromBitmapData(stage.Dis, rootwin, 
	       (char *)empty, 2, 2, 1, 0, 1);
  crs = XCreatePixmapCursor(stage.Dis, pm, pm, &curcol, &curcol, 0, 0);
  XFreePixmap(stage.Dis, pm);
 
  xswa.cursor = crs;
  xswa.background_pixel = stage.black;
  xswa.override_redirect = True;
  xswa.backing_store = NotUseful;
  xswa.event_mask = ExposureMask | KeyPressMask | EnterWindowMask ;

  if(stage.depth > 1 && options.color)
    {
      stage.white = 3;
      stage.black = 0;
      stage.red = 255;
      xswa.background_pixel = stage.black;

      dmap = DefaultColormap(stage.Dis, stage.Sc);
      if (XMatchVisualInfo(stage.Dis, stage.Sc, 
		DisplayPlanes(stage.Dis, stage.Sc), PseudoColor, &vinfo)) 
	{
	  stage.vis = vinfo.visual;
	}
      else
	{
	  printf("Not ready for non - PseudoColor display\n");
	  stage.vis = DefaultVisual(stage.Dis, stage.Sc);
	}
      xmap = XCreateColormap(stage.Dis, RootWindow(stage.Dis, stage.Sc), 
		     stage.vis, AllocNone);
      if(! XAllocColorCells(stage.Dis, xmap, False, &pmask, 0, pixels, 
		 vinfo.colormap_size))
	{
	  printf("Can't get enough colors!\n");
	  return;
	}
      xswa.colormap = xmap;
    }
#if USE_PIXMAP_IF_COLOR
  else if(stage.depth > 1)
    {
      stage.depth = 1;
      stage.red = stage.white;
      stage.vis = DefaultVisual(stage.Dis, stage.Sc);
    }
#endif
  else
    {
      stage.red = stage.white;
      stage.vis = DefaultVisual(stage.Dis, stage.Sc);
    }

  stage.Win = XCreateWindow(stage.Dis, rootwin, 
                      0, 0, width, height, 0, CopyFromParent,
                      InputOutput, CopyFromParent,
                      CWEventMask | CWCursor | CWBackPixel | 
		      CWBackingStore |
		      ((options.color && stage.depth > 1) ? CWColormap : 0) | 
		      (options.nolock ? 0 : CWOverrideRedirect) ,
                      &xswa);
  wmh.input = True;
  XSetWMHints(stage.Dis, stage.Win, &wmh);

  gcval.background = stage.black; gcval.foreground = stage.white;
  gcval.font = fnt->fid;
  stage.wonb = XCreateGC(stage.Dis, stage.Win,
			 GCBackground | GCForeground | GCFont, &gcval);
  gcval.background =  stage.white; gcval.foreground = stage.black ;
  stage.bonw = XCreateGC(stage.Dis, stage.Win,
			 GCBackground | GCForeground | GCFont, &gcval);
  gcval.font = fntl->fid;
  stage.bonwl = XCreateGC(stage.Dis, stage.Win,
			 GCBackground | GCForeground | GCFont, &gcval);

  /* 
   * open the window 
   * wait until we get exposed, draw the clock with its hand on 15.
   */
  debug1("Init %d\n", time(0));
  Init();

  /*
   * initialize timer, 
   * load the animation sequence and start countdown.
   */
  mu(0, 0); 
  debug1("initgraphics %d\n", time(0));
  if (initgraphics() != 0)
    options.noanim = 1;
  if (!stage.ticks)
    {
      /* loaded by initstage() called from Init(); */
      fprintf(stderr, "Sorry, cannot even move hand!\n");
      free_all();
    }
  /*
   * for all ticks left, block the needed time, and show them
   */

  if (stage.ticks->frame > 0)
    mu(1000 * options.speed, MU_BLOCK);
  end = 0;
  while (!end)
    {
      end = DrawFrameGrid(stage.ticks, stage.grid);
      UpdateDisplayFromGrid();
      if (!end)
	{
          EraseFrameGrid(stage.ticks, stage.grid);
	  UpdateWorkFromBack();
          mu(1000 * options.speed, MU_BLOCK);
	}
      if (options.speed >= 60 && stage.ticks->frame == 5)
	{
	  /* as soon as the hand hits the zero... */
	  sprintf(logbuf, "PARKLOCK: parktime exceeded for %s\n",
		  LoginName);
	  syslog(LOG_NOTICE, logbuf);
	  /*
	   * negative time is always one minute per tick
	   */
	  if (options.speed > 60)
	    options.speed = 60;
	}
    }

  if (!options.noanim)
    {
      play_animation();
    }
  if (options.harmless)
    free_all();
  else
    plock_terminate();
}

static void Init()
{
  XEvent  event;

  if (initstage() != 0)
    free_all();

  XMapWindow(stage.Dis, stage.Win);

  if (!options.nolock)
    XRaiseWindow(stage.Dis, stage.Win);
  else
    XLowerWindow(stage.Dis, stage.Win);

  lock();

  do
    XNextEvent(stage.Dis, &event);
  while (event.type != Expose);
  /* this even sets LoginName... */
  if (stage.depth > 1 && options.color && !options.nolock)
    {
      XInstallColormap(stage.Dis, xmap);
      XSync(stage.Dis, False);
    }
  drawinit();
}


extern int select __P((int, fd_set *, fd_set *, fd_set *, struct timeval *));

#define TIMELEFT(t1, t2) ((t1.tv_sec > t2.tv_sec) || ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec >= t2.tv_usec)))

/*
 * to is timeout in milliseconds.
 */
int
mu(to, poll)
int to, poll;
{
  int j, exceed = 0;
  fd_set read_fd;
  struct timeval tv, end, now;
  struct timezone tz;
  static struct timeval tstamp;

  if (!to)
    {
      /*
       * initialize our timestamp without looking at it.
       */
      gettimeofday(&tstamp, &tz);
      return 1;
    }

  gettimeofday(&now, &tz);
  end.tv_sec = tstamp.tv_sec + to / 1000;
  end.tv_usec = tstamp.tv_usec + (to % 1000) * 1000;
  while (end.tv_usec > 999999)
    {
      end.tv_usec -= 1000000;
      end.tv_sec++;
    }

  tv.tv_sec = tv.tv_usec = 0;
  if (TIMELEFT(end, now)) 
    {
      if (poll == MU_BLOCK)
	{
	  /*
	   * user called us early enough
	   * and wants us to sleep.
	   */
	  tv.tv_sec = end.tv_sec - now.tv_sec;
	  tv.tv_usec = end.tv_usec - now.tv_usec;
	  while (tv.tv_usec < 0)
	    {
	      tv.tv_usec += 1000000;
	      tv.tv_sec--;
	    }
	  exceed = 1;
	}
    }
  else
    exceed = 1;

  FD_ZERO(&read_fd);
  FD_SET(stage.Dis->fd, &read_fd);

  errno = 0;
  while ((j = select(FD_SETSIZE, &read_fd, 0, 0, &tv)) > 0)
    {
      eat_events();
      gettimeofday(&now, &tz);
      if ((poll == MU_BLOCK) && TIMELEFT(end, now)) 
        {
          /*
           * still too early, continue sleeping.
           */
          tv.tv_sec = end.tv_sec - now.tv_sec;
          tv.tv_usec = end.tv_usec - now.tv_usec;
          while (tv.tv_usec < 0)
	    {
	      tv.tv_usec += 1000000;
	      tv.tv_sec--;
	    }
          FD_ZERO(&read_fd);
	  FD_SET(stage.Dis->fd, &read_fd);
	}
      else
	tv.tv_sec = tv.tv_usec = 0;
    }
  if (j < 0)
    perror("select");
  if (exceed)
    {
      tstamp = end;
      return 1;
    }
  return 0;
}

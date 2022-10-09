#define USE_COLOR 1

#ifdef sun
# ifdef SOLARIS

/* leftmost column is OFFSET 0 */
#define PSPROG "/bin/ps"
#define PSARGS "-le"
#define PS_UID_OFFSET  4
#define PS_PID_OFFSET 10
#define PS_TTY_OFFSET 58
#define PS_AV0_OFFSET 72

#define FONT "-*-lucida-medium-r-*-sans-24-*-*-*-*-*-*-*"
#define FONTL "-*-lucida-bold-r-*-sans-14-*-*-*-*-*-*-*"

#include <sys/kbd.h>
#include <sys/kbio.h>

# else

#define PSPROG "/bin/ps"
#define PSARGS "-lx"
#define PS_UID_OFFSET  8
#define PS_PID_OFFSET (13 + ((getuid() > 9999)? 1 : 0))
#define PS_TTY_OFFSET (58 + ((getuid() > 9999)? 1 : 0))
#define PS_AV0_OFFSET (67 + ((getuid() > 9999)? 1 : 0))

#define FONT "-*-lucida-medium-r-*-sans-24-*-*-*-*-*-*-*"
#define FONTL "-*-lucida-bold-r-*-sans-14-*-*-*-*-*-*-*"

#include <sundev/kbd.h>
#include <sundev/kbio.h>

# endif
#endif

#ifdef hpux

#define PSPROG "/bin/ps"
#define PSARGS "-le"
#define PS_UID_OFFSET  6
#define PS_PID_OFFSET 12
#define PS_TTY_OFFSET 57
#define PS_AV0_OFFSET 71

#define X_NOT_ON_CONSOLE

#define FONT "-*-*-bold-r-normal--24-*-*-*-*-*-iso8859-*"
#define FONTL "-*-*-medium-r-normal--12-*-*-*-*-*-iso8859-*"

#include <sys/stat.h>

#endif

#ifdef linux

#define PSPROG "/usr/bin/ps"
#define PSARGS "-lx"
#define PS_UID_OFFSET 2
#define PS_PID_OFFSET 8
#define PS_TTY_OFFSET 54
#define PS_AV0_OFFSET 64
#define X_NOT_ON_CONSOLE
#define FONT "-*-lucida-medium-r-*-sans-24-*-*-*-*-*-*-*"
#define FONTL "-*-lucida-bold-r-*-sans-14-*-*-*-*-*-*-*"

#endif

#ifdef sgi

#define PSPROG "/bin/ps"
#define PSARGS "-le"
#define PS_UID_OFFSET  4
#define PS_PID_OFFSET 10
#define PS_TTY_OFFSET 53
#define PS_AV0_OFFSET 68

#undef USE_COLOR
#define USE_COLOR 1

#define DELETE_PROPERTY "_SGI_SESSION_PROPERTY"

#define FONT "-*-*-bold-r-normal--24-*-*-*-*-*-iso8859-*"
#define FONTL "-*-*-medium-r-normal--12-*-*-*-*-*-iso8859-*"

#include <sys/stat.h>

#endif

#ifdef __osf__

#define PSPROG "/bin/ps"
#define PSARGS "-le"
#define PS_UID_OFFSET 14
#define PS_PID_OFFSET 20
#define PS_TTY_OFFSET 59
#define PS_AV0_OFFSET 75

#define X_NOT_ON_CONSOLE

#define FONT "-*-lucida-medium-r-*sans-24-*-*-*-*-*-*-*"
#define FONTL "-*-lucida-bold-r-*sans-14-*-*-*-*-*-*-*"

#include <sys/stat.h>

#ifndef MAXPID
#define MAXPID PID_MAX
#endif

#endif


#ifdef _IBMR2

#define PSPROG "/bin/ps"
#define PSARGS "-le"
#define PS_UID_OFFSET 11
#define PS_PID_OFFSET 17
#define PS_TTY_OFFSET 61
#define PS_AV0_OFFSET 73

#define X_NOT_ON_CONSOLE

#define FONT "-*-lucida-medium-r-*sans-24-*-*-*-*-*-*-*"
#define FONTL "-*-lucida-bold-r-*sans-14-*-*-*-*-*-*-*"

#include <sys/stat.h>
#include <sys/select.h>

#endif

#ifndef PSPROG
#include "-- Sorry, plock has not been adapted for this architecture."
#include "-- See config.h for a few examples and build an own entry."
#include "-- Please send your changes back to the authors!"
#endif

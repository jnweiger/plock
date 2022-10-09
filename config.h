#ifdef sun

#define PSPROG "/bin/ps"
#define PSARGS "-lx"
#define PS_UID_OFFSET  8
#define PS_PID_OFFSET (13 + ((getuid() > 9999)? 1 : 0))
#define PS_TTY_OFFSET (58 + ((getuid() > 9999)? 1 : 0))
#define PS_AV0_OFFSET (67 + ((getuid() > 9999)? 1 : 0))

#define FONT "-*-lucida-medium-r-*sans-24-*-*-*-*-*-*-*"
#define FONTL "-*-lucida-bold-r-*sans-14-*-*-*-*-*-*-*"

#include <sundev/kbd.h>
#include <sundev/kbio.h>

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

/*
 * Copyright (C) 2000-2007 Carsten Haitzler, Geoff Harrison and various contributors
 * Copyright (C) 2004-2015 Kim Woelders
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "config.h"

#include <time.h>
#include <sys/time.h>
#include <X11/Xlib.h>

#include "E.h"
#include "xwin.h"

/*
 * Debug/error message printing.
 */
static struct timeval tv0 = { 0, 0 };

static void
_tvdiff(struct timeval *tvd, const struct timeval *tv1,
	const struct timeval *tv2)
{
   long                tsec, tus;

   tsec = tv2->tv_sec - tv1->tv_sec;
   tus = tv2->tv_usec - tv1->tv_usec;
   if (tus < 0)
     {
	tus += 1000000;
	tsec -= 1;
     }
   tvd->tv_sec = tsec;
   tvd->tv_usec = tus;
}

void
Eprintf(const char *fmt, ...)
{
   FILE               *fprt;
   va_list             args;
   struct timeval      tv;

   if (tv0.tv_sec == 0)
      gettimeofday(&tv0, NULL);

   fprt = Conf.log.dest ? stderr : stdout;

   gettimeofday(&tv, NULL);
   _tvdiff(&tv, &tv0, &tv);

   if (Conf.log.difftime)
     {
	static struct timeval tv1 = { 0, 0 };
	struct timeval      tvd;
	unsigned long       nreq;

	_tvdiff(&tvd, &tv1, &tv);
	tv1 = tv;

	nreq = (disp) ? NextRequest(disp) : 0;
	fprintf(fprt, "[%d] %#8lx %4ld.%06ld [%3ld.%06ld]: ", getpid(), nreq,
		(long)tv1.tv_sec, tv1.tv_usec, (long)tvd.tv_sec, tvd.tv_usec);
     }
   else
     {
	fprintf(fprt, "[%d] %4ld.%06ld: ", getpid(),
		(long)tv.tv_sec, tv.tv_usec);
     }

   va_start(args, fmt);
   vfprintf(fprt, fmt, args);
   va_end(args);
}

#if ENABLE_DEBUG_EVENTS
/*
 * Event debug stuff
 */
#define N_DEBUG_FLAGS 256
static char         ev_debug;
static char         ev_debug_flags[N_DEBUG_FLAGS];

/*
 * param is <ItemNumber>[:<ItemNumber> ... ]
 *
 * ItemNumber:
 * 0            : Verbose flag
 * 1            : X11 errors
 * [   2;  35 [ : X11 event codes, see /usr/include/X11/X.h
 * [  64; ... [ : Remapped X11 events, see events.h
 * [ 128; 256 [ : E events, see E.h
 */
void
EDebugInit(const char *param)
{
   const char         *s;
   int                 ix, onoff;

   if (!param)
      return;

   for (;;)
     {
	s = strchr(param, ':');
	if (!param[0])
	   break;
	ev_debug = 1;
	ix = strtol(param, NULL, 0);
	onoff = (ix >= 0);
	if (ix < 0)
	   ix = -ix;
	if (ix < N_DEBUG_FLAGS)
	  {
	     if (onoff)
		ev_debug_flags[ix]++;
	     else
		ev_debug_flags[ix] = 0;
	  }
	if (!s)
	   break;
	param = s + 1;
     }
}

int
EDebug(unsigned int type)
{
   return (ev_debug &&
	   (type < sizeof(ev_debug_flags))) ? ev_debug_flags[type] : 0;
}

void
EDebugSet(unsigned int type, int value)
{
   if (type >= sizeof(ev_debug_flags))
      return;

   ev_debug = 1;
   ev_debug_flags[type] += value;
}

#endif

#if USE_MODULES
/*
 * Dynamic module loading
 */
#include <dlfcn.h>

const void         *
ModLoadSym(const char *lib, const char *sym, const char *name)
{
   char                buf[1024];
   void               *h;

   Esnprintf(buf, sizeof(buf), "%s/lib%s_%s.so", EDirLib(), lib, name);
   if (EDebug(1))
      Eprintf("%s: %s\n", __func__, buf);
   h = dlopen(buf, RTLD_NOW | RTLD_LOCAL);
   if (!h)
      Eprintf("*** %s: %s: %s\n", __func__, buf, dlerror());
   if (!h)
      return NULL;

   Esnprintf(buf, sizeof(buf), "%s_%s", sym, name);
   h = dlsym(h, buf);
   if (!h)
      Eprintf("*** %s: %s: %s\n", __func__, buf, dlerror());

   return h;
}

#endif /* USE_MODULES */
